$src = $args[0]
$dst = $args[1] 

New-Item -ItemType SymbolicLink -Path $dst -Target $src
