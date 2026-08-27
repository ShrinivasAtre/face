param([Parameter(Mandatory=$true)][string]$Annotations,[Parameter(Mandatory=$true)][string]$OutputCsv,[double]$DefaultToleranceMs=100)
$ErrorActionPreference='Stop';$rows=@(Import-Csv -LiteralPath $Annotations|Where-Object { $_.label_quality -ne 'uncertain' })
$annotators=@($rows.annotator_id|Sort-Object -Unique);if($annotators.Count -ne 2){throw 'Agreement scoring requires exactly two annotators'}
$a=@($rows|Where-Object { $_.annotator_id -eq $annotators[0] });$b=@($rows|Where-Object { $_.annotator_id -eq $annotators[1] });$out=@()
$keys=@($rows|ForEach-Object{"$($_.clip_id)|$($_.event)|$($_.side)"}|Sort-Object -Unique)
foreach($key in $keys){$k=$key-split'\|',3;$aa=@($a|Where-Object{$_.clip_id-eq$k[0]-and$_.event-eq$k[1]-and$_.side-eq$k[2]});$bb=@($b|Where-Object{$_.clip_id-eq$k[0]-and$_.event-eq$k[1]-and$_.side-eq$k[2]});$used=@{};$matched=0;$onset=@();$offset=@()
    foreach($x in $aa){$tol=if($x.boundary_tolerance_ms-ne''){[double]$x.boundary_tolerance_ms}else{$DefaultToleranceMs};$best=$null;$dist=[double]::PositiveInfinity
        for($i=0;$i-lt$bb.Count;++$i){if($used.ContainsKey($i)){continue};$d=[math]::Abs([double]$bb[$i].start_ms-[double]$x.start_ms);$overlap=[math]::Min([double]$x.end_ms,[double]$bb[$i].end_ms)-[math]::Max([double]$x.start_ms,[double]$bb[$i].start_ms);if(($overlap-gt0-or$d-le$tol)-and$d-lt$dist){$best=$i;$dist=$d}}
        if($null-ne$best){$used[$best]=$true;++$matched;$onset+=$dist;$offset+=[math]::Abs([double]$bb[$best].end_ms-[double]$x.end_ms)}
    }
    $denom=$aa.Count+$bb.Count;$out+=[pscustomobject]@{clip_id=$k[0];event=$k[1];side=$k[2];annotator_a_count=$aa.Count;annotator_b_count=$bb.Count;matched=$matched;event_f1=if($denom){[math]::Round(2*$matched/$denom,6)}else{1};mean_absolute_onset_difference_ms=if($onset.Count){[math]::Round(($onset|Measure-Object -Average).Average,3)}else{''};mean_absolute_offset_difference_ms=if($offset.Count){[math]::Round(($offset|Measure-Object -Average).Average,3)}else{''};requires_adjudication=if($matched-lt[math]::Max($aa.Count,$bb.Count)-or@($onset|Where-Object{$_-gt$DefaultToleranceMs}).Count){1}else{0}}
}
$out|Export-Csv -NoTypeInformation -LiteralPath $OutputCsv
