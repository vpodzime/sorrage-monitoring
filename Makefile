all: md_report desktop_notify

configure:
	@pkg-config libsystemd
	@echo "pkg-config libsystemd...ok"

md_report: src/md_report.c
	gcc -o md_report `pkg-config --libs --cflags libsystemd` src/md_report.c

desktop_notify: src/desktop_notify.c
	gcc -o d_notify `pkg-config --libs --cflags libsystemd` src/desktop_notify.c
