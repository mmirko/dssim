all : talk.pdf alone

%.pdf : %.tex
	@pdflatex $^
	@pdflatex $^

alone:
	@echo "Removing ..." && rm -rf *.log *aux *.nav *.out *.snm *.toc && echo "... ok"

clean :
	@echo "Removing ..." && rm -rf *.log *aux *.nav *.out *.snm *.toc *.pdf && echo "... ok"
