all : article.pdf alone

%.pdf : %.tex
	@pdflatex $^
	@pdflatex $^
	@bibtex article
	@pdflatex $^
	@pdflatex $^

alone:
	@echo "Removing ..." && rm -rf *.log *.nav *.out *.snm *.toc && echo "... ok"

clean :
	@echo "Removing ..." && rm -rf *.log *aux *.nav *.out *.snm *.toc *.pdf *.blg && echo "... ok"

