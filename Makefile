.PHONY: docs docs-pdf docs-web docs-serve screenshots test

# Versión y fecha de la última entrada de CHANGELOG.md («## [0.1.0] - 2026-11-10»).
VERSION  := $(shell grep -m1 -E '^\#\# \[[0-9]' CHANGELOG.md | sed -E 's/^\#\# \[([^]]+)\].*/\1/')
PUB_DATE := $(shell grep -m1 -E '^\#\# \[[0-9]' CHANGELOG.md | sed -E 's/.* - ([0-9]+)-([0-9]+)-([0-9]+)/\3\/\2\/\1/')

## Genera el manual completo: PDF y web.
docs: docs-pdf docs-web

## Manual en PDF (docs/manual/esp32-chess-manual.pdf): pandoc (Markdown -> HTML) y pagedjs-cli (Chrome).
docs-pdf:
	@command -v pandoc >/dev/null 2>&1 || { echo "Necesitas pandoc (brew install pandoc)."; exit 1; }
	@command -v npx >/dev/null 2>&1 || { echo "Necesitas Node.js/npx."; exit 1; }
	pandoc -s -f markdown-auto_identifiers --toc --toc-depth=2 \
		--lua-filter=docs/pandoc-admonitions.lua \
		--lua-filter=docs/pandoc-internal-links.lua \
		--metadata title="Ajedrez de los animales" \
		--metadata subtitle="Manual para peques, familias y docentes" \
		--metadata date="Versión $(VERSION) · $(PUB_DATE)" \
		--metadata lang=es \
		-c assets/theme.css -c assets/print.css \
		-o docs/manual/_build.html \
		docs/manual/index.md docs/manual/[0-9][0-9]-*.md
	cd docs/manual && CHROME="$${PUPPETEER_EXECUTABLE_PATH:-$$(for c in \
		"/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" \
		"/Applications/Chromium.app/Contents/MacOS/Chromium" \
		"$$(command -v google-chrome)" "$$(command -v chromium)" "$$(command -v chromium-browser)"; do \
		[ -x "$$c" ] && echo "$$c" && break; done)}"; \
		PUPPETEER_EXECUTABLE_PATH="$$CHROME" npx --yes pagedjs-cli _build.html -o esp32-chess-manual.pdf

## Web del manual (docs/manual-site/) con MkDocs Material: pip install -r docs/manual/requirements.txt
docs-web:
	@command -v mkdocs >/dev/null 2>&1 || { echo "Necesitas MkDocs Material: pip install -r docs/manual/requirements.txt"; exit 1; }
	MANUAL_COPYRIGHT="Ajedrez de los animales · versión $(VERSION) · $(PUB_DATE)" mkdocs build -f docs/manual/mkdocs.yml

## Previsualiza la web en http://127.0.0.1:8000
docs-serve:
	MANUAL_COPYRIGHT="Ajedrez de los animales · versión $(VERSION) · $(PUB_DATE)" mkdocs serve -f docs/manual/mkdocs.yml

## Regenera las capturas del manual con el simulador del PC (no hace falta la placa).
screenshots:
	@for s in start newgame newgame_two newgame_pass newgame_clock select mid capture check hint promo win danger adapt names_accents slots slots_confirm two_black two_pass two_flag two_promo; do \
		tools/sim/shot.sh $$s "$(CURDIR)/docs/manual/img/$$s.png" >/dev/null || exit 1; done

test:
	~/.platformio/penv/bin/pio test -e native
