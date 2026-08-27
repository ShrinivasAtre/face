param([Parameter(Mandatory=$true)][string]$Annotations,[string]$OutputCsv='')
$ErrorActionPreference='Stop'
$rows=@(Import-Csv -LiteralPath $Annotations)
if(-not$rows.Count){throw 'Annotation file has no rows'}
$required=@('clip_id','subject_id','session_id','event','side','start_ms','end_ms','label_quality','boundary_tolerance_ms','annotator_id','notes')
foreach($c in $required){if($rows[0].PSObject.Properties.Name-notcontains$c){throw "Annotation file missing column: $c"}}
$allowedQuality=@('accepted','uncertain','adjudicated');$errors=@();$seen=@{}
for($i=0;$i-lt$rows.Count;++$i){$r=$rows[$i];$line=$i+2;$start=0.0;$end=0.0;$tol=0.0
    if([string]::IsNullOrWhiteSpace($r.clip_id)-or[string]::IsNullOrWhiteSpace($r.event)-or[string]::IsNullOrWhiteSpace($r.annotator_id)){$errors+=[pscustomobject]@{line=$line;error='required value is empty'}}
    if(-not[double]::TryParse($r.start_ms,[ref]$start)-or-not[double]::TryParse($r.end_ms,[ref]$end)-or$start-lt0-or$end-le$start){$errors+=[pscustomobject]@{line=$line;error='invalid half-open interval'}}
    if(-not[double]::TryParse($r.boundary_tolerance_ms,[ref]$tol)-or$tol-lt0){$errors+=[pscustomobject]@{line=$line;error='invalid boundary tolerance'}}
    if($r.label_quality-notin$allowedQuality){$errors+=[pscustomobject]@{line=$line;error='invalid label_quality'}}
    $key="$($r.clip_id)|$($r.event)|$($r.side)|$($r.start_ms)|$($r.end_ms)|$($r.annotator_id)";if($seen.ContainsKey($key)){$errors+=[pscustomobject]@{line=$line;error='duplicate annotation'}}else{$seen[$key]=$true}
}
if($OutputCsv){$errors|Export-Csv -NoTypeInformation -LiteralPath $OutputCsv}
if($errors.Count){throw "Annotation validation failed with $($errors.Count) error(s)"}
[pscustomobject]@{rows=$rows.Count;clips=@($rows.clip_id|Sort-Object -Unique).Count;annotators=@($rows.annotator_id|Sort-Object -Unique).Count}|ConvertTo-Json
