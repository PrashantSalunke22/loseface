ifndef PYTHON
PYTHON = C:/Python26/python.exe
endif

RST2HTML = docutils/tools/rst2html.py

default: all

all:
	$(PYTHON) $(RST2HTML) --config manual.conf manual.rst > manual.html

clean:
	rm *.html
