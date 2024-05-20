# compile.ps1
$includeDir = ".\include"
$libDir = ".\lib"
$fileName = "main"

gcc -o main main.c -IC:$includeDir -LC:$libDir -lmingw32 -lSDL2main -lSDL2

# Check if the gcc command was successful
if ($LASTEXITCODE -eq 0) {
    Write-Output "Compilation successful. Running window.exe..."
    .\main.exe
} else {
    Write-Output "Compilation failed. window.exe will not be run."
}