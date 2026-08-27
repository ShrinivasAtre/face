param(
    [Parameter(Mandatory=$true)] [string] $GroundTruth,
    [Parameter(Mandatory=$true)] [string] $Predictions,
    [Parameter(Mandatory=$true)] [string] $OutputCsv
)
$ErrorActionPreference='Stop'
$truth=@(Import-Csv -LiteralPath $GroundTruth | Where-Object label_quality -ne 'uncertain')
$pred=@(Import-Csv -LiteralPath $Predictions)
foreach($set in @(@($truth,'Ground truth'),@($pred,'Predictions'))){
    foreach($column in @('clip_id','state_type','side','state','start_ms','end_ms')){
        if($set[0].Count -and $set[0][0].PSObject.Properties.Name -notcontains $column){throw "$($set[1]) missing column: $column"}
    }
}
$out=@()
$keys=@($truth|ForEach-Object{"$($_.clip_id)|$($_.state_type)|$($_.side)"}|Sort-Object -Unique)
foreach($key in $keys){
    $k=$key-split '\|',3
    $t=@($truth|Where-Object{$_.clip_id-eq$k[0]-and$_.state_type-eq$k[1]-and$_.side-eq$k[2]})
    $p=@($pred|Where-Object{$_.clip_id-eq$k[0]-and$_.state_type-eq$k[1]-and$_.side-eq$k[2]})
    $bounds=@($t+$p|ForEach-Object{[double]$_.start_ms;[double]$_.end_ms}|Sort-Object -Unique)
    $known=0.0;$correct=0.0;$unknown=0.0;$total=0.0
    for($i=0;$i-lt$bounds.Count-1;++$i){
        $a=$bounds[$i];$b=$bounds[$i+1];if($b-le$a){continue};$mid=($a+$b)/2;$d=$b-$a
        $tv=@($t|Where-Object{[double]$_.start_ms-le$mid-and[double]$_.end_ms-gt$mid}|Select-Object -First 1)
        if(-not$tv.Count){continue};$total+=$d
        $pv=@($p|Where-Object{[double]$_.start_ms-le$mid-and[double]$_.end_ms-gt$mid}|Select-Object -First 1)
        if(-not$pv.Count-or$pv[0].state-eq'unknown'){$unknown+=$d;continue}
        $known+=$d;if($pv[0].state-eq$tv[0].state){$correct+=$d}
    }
    $out += [pscustomobject]@{clip_id=$k[0];state_type=$k[1];side=$k[2];truth_duration_ms=[math]::Round($total,3);known_duration_ms=[math]::Round($known,3);unknown_duration_ms=[math]::Round($unknown,3);known_coverage=if($total){[math]::Round($known/$total,6)}else{0};conditional_accuracy=if($known){[math]::Round($correct/$known,6)}else{0}}
}
$out|Export-Csv -NoTypeInformation -LiteralPath $OutputCsv
