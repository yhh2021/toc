all:
	c89 toc.c -Wall

debug:
	c89 toc.c -g
	gdb a.out

callgraph:
	c89 -fdump-rtl-expand toc.c
	egypt --include-external toc.c.233r.expand > toc.c.call.dot
	dot -Grankdir=LR -Tsvg -O toc.c.call.dot
	xdg-open toc.c.call.dot.svg

release:
	cproto toc.c > toc.proto
