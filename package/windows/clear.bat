@echo off

if exist %1 (
	rd %1 /s /q
	echo "���%1"
)
md %1