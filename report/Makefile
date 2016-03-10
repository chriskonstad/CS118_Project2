TEX=report.tex

../report.pdf: $(TEX)
	latexmk -gg -pdf $<
	mv report.pdf ../report.pdf

clean:
	latexmk -C $(TEX)
