TEX=report.tex

report.pdf: $(TEX)
	latexmk -pdf $<

clean:
	latexmk -C $(TEX)
