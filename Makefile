all:
	c89 toc.c

debug:
	c89 toc.c -g

callgraph:
	c89 -fdump-rtl-expand toc.c
	egypt toc.c.233r.expand > toc.c.call.dot
	dot -Tsvg -O toc.c.call.dot
	xdg-open toc.c.call.dot.svg
