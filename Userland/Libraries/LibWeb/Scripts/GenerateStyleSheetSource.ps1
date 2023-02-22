#get args
$1 = $args[0]
$2 = $args[1]

echo "namespace Web::CSS {"
echo "extern const char $1[];"
echo "const char $1[] = R`"`"`"("
Get-Content $2 | ForEach-Object {
	if($_ -notmatch "^ *#") {
		echo $_""
	}
}
echo ")`"`"`";"
echo "}"
