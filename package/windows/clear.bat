@echo off

if exist %1 (
	rd %1 /s /q
	echo "Çå¿Õ%1"
)
md %1