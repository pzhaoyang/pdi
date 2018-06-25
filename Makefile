src := client.c
hdr := include

.PHONY: debug_tool

debug_tool: $(src) $(hdr)

.PHONY: clean
clean:
	@echo remove all generate files