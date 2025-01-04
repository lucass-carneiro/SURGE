
$SourceFolder = $args[0]
$DestinationFolder = $args[1] 


Get-ChildItem -Path $SourceFolder -Filter "*.dll" | ForEach-Object {
  $SourcePath = $_.FullName
  $LinkPath = Join-Path -Path $DestinationFolder -ChildPath $_.Name
  New-Item -ItemType SymbolicLink -Path $LinkPath -Target $SourcePath
}
