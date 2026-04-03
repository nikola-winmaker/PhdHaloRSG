# Create virtual environment
python -m venv halo-env
./halo-env/Scripts/Activate.ps1

# Install HALO framework and generators from local .whl files
$haloDir = "./halo"

# Install main HALO framework
$mainWheel = Get-ChildItem -Path $haloDir -Filter "halo-*.whl" -ErrorAction SilentlyContinue | Where-Object { $_.Name -notlike "*halo_gen*" -and $_.Name -notlike "*halo_proto*" } | Select-Object -First 1

if ($mainWheel) {
    Write-Host "Installing HALO framework: $($mainWheel.Name)" -ForegroundColor Green
    pip install --force-reinstall $mainWheel.FullName
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "HALO framework installation failed!" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "No halo-*.whl file found in $haloDir" -ForegroundColor Red
    Write-Host "Please place the HALO framework .whl file in the $haloDir directory" -ForegroundColor Yellow
    exit 1
}

# Install generators
$generators = Get-ChildItem -Path $haloDir -Filter "halo_gen*.whl" -ErrorAction SilentlyContinue

if ($generators) {
    Write-Host "Found $($generators.Count) generator(s), installing..." -ForegroundColor Green
    foreach ($gen in $generators) {
        Write-Host "Installing: $($gen.Name)" -ForegroundColor Cyan
        pip install --force-reinstall $gen.FullName
    }
} else {
    Write-Host "No generator .whl files found in $haloDir" -ForegroundColor Yellow
}

# Install protocol generators if available
$protocols = Get-ChildItem -Path $haloDir -Filter "halo_proto*.whl" -ErrorAction SilentlyContinue

if ($protocols) {
    Write-Host "Found $($protocols.Count) protocol generator(s), installing..." -ForegroundColor Green
    foreach ($proto in $protocols) {
        Write-Host "Installing: $($proto.Name)" -ForegroundColor Cyan
        pip install --force-reinstall $proto.FullName
    }
}
