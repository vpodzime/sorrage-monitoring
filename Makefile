all: md_report

configure:
	@pkg-config libsystemd
	@echo "pkg-config libsystemd...ok"

md_report: src/md_report.c
	gcc -o md_report `pkg-config --libs --cflags libsystemd` src/md_report.c
