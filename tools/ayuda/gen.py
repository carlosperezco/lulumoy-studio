"""Genera la guía matemática de Lulumoy Studio (help/<idioma>/*.html).

Uso, desde la carpeta del proyecto:
    python tools/ayuda/gen.py

Los textos están en pages_*.py, en español, inglés e italiano; las fórmulas,
en MathML (ver mathml.py). Los nombres de los parámetros se leen de lang/*.lang,
así que coinciden siempre con los de los diálogos. Al añadir un módulo con
parámetros, añada su página en el archivo pages_*.py que corresponda y en
GROUPS (más abajo), y vuelva a ejecutar este script.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from mathml import *  # noqa: E402,F401
import pages_filters  # noqa: E402
import pages_geometry  # noqa: E402
import pages_vector  # noqa: E402

ROOT = sys.argv[1] if len(sys.argv) > 1 else os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
LANGS = ('es', 'en', 'it')

UI = {
    'guide': {'es': 'Guía matemática', 'en': 'Mathematical guide', 'it': 'Guida matematica'},
    'back': {'es': '← Índice', 'en': '← Contents', 'it': '← Indice'},
    'params': {'es': 'Parámetros', 'en': 'Parameters', 'it': 'Parametri'},
    'param': {'es': 'Parámetro', 'en': 'Parameter', 'it': 'Parametro'},
    'meaning': {'es': 'Significado', 'en': 'Meaning', 'it': 'Significato'},
    'see': {'es': 'Véase también', 'en': 'See also', 'it': 'Vedi anche'},
    'note': {'es': 'Nota', 'en': 'Note', 'it': 'Nota'},
    'geometry': {'es': 'Geometría', 'en': 'Geometry', 'it': 'Geometria'},
    'format': {'es': 'Formato', 'en': 'Format', 'it': 'Formato'},
    'filters': {'es': 'Filtros', 'en': 'Filters', 'it': 'Filtri'},
    'vector': {'es': 'Vectorización', 'en': 'Vectorization', 'it': 'Vettorializzazione'},
    'notation': {'es': 'Notación', 'en': 'Notation', 'it': 'Notazione'},
    'intro': {
        'es': 'Esta guía explica qué calcula cada operación de Lulumoy Studio y cómo influye cada parámetro. Las fórmulas siguen las definiciones de las funciones de OpenCV que usa el programa. También se abre con el botón <b>Ayuda</b> de cada diálogo de parámetros y con F1.',
        'en': 'This guide explains what each Lulumoy Studio operation computes and how each parameter affects it. The formulas follow the definitions of the OpenCV functions the program uses. It also opens from the <b>Help</b> button of every parameter dialog and with F1.',
        'it': 'Questa guida spiega che cosa calcola ogni operazione di Lulumoy Studio e come influisce ogni parametro. Le formule seguono le definizioni delle funzioni di OpenCV usate dal programma. Si apre anche con il pulsante <b>Aiuto</b> di ogni dialogo dei parametri e con F1.',
    },
    'notation_text': {
        'es': 'I(x, y) es el valor del píxel de la columna x y la fila y, con el origen en la esquina superior izquierda; I′ es el resultado. W×H es el tamaño de la imagen y M el valor máximo de su profundidad (255 en 8 bits, 65535 en 16 bits). ∗ indica convolución. Salvo que se diga lo contrario, en las imágenes en color cada canal se procesa por separado.',
        'en': 'I(x, y) is the value of the pixel in column x and row y, with the origin at the top-left corner; I′ is the result. W×H is the image size and M the maximum value of its bit depth (255 for 8 bits, 65535 for 16 bits). ∗ denotes convolution. Unless stated otherwise, colour images are processed channel by channel.',
        'it': "I(x, y) è il valore del pixel della colonna x e della riga y, con l'origine nell'angolo superiore sinistro; I′ è il risultato. W×H è la dimensione dell'immagine e M il valore massimo della sua profondità (255 a 8 bit, 65535 a 16 bit). ∗ indica la convoluzione. Salvo indicazione contraria, nelle immagini a colori ogni canale si elabora separatamente.",
    },
}

GROUPS = [
    ('geometry', ['edit.rotate', 'edit.flip', 'edit.crop', 'edit.affine', 'edit.perspective', 'interpolation']),
    ('format', ['format']),
    ('filters', ['filter.gaussian', 'filter.box', 'filter.median', 'filter.bilateral', 'filter.sharpen', 'filter.canny',
                 'filter.threshold', 'filter.brightness_contrast', 'filter.morphology']),
    ('vector', ['vectorize']),
]

CSS = """
:root { color-scheme: light dark; --acento: #2e7d32; --suave: #f3f8ee; --borde: #cfe3c0; }
@media (prefers-color-scheme: dark) { :root { --acento: #8bd450; --suave: #1e261a; --borde: #3a4a30; } }
body { font-family: "Segoe UI", system-ui, -apple-system, Roboto, Ubuntu, sans-serif; line-height: 1.55;
       max-width: 860px; margin: 0 auto; padding: 1.5rem 1.25rem 3rem; }
header { display: flex; align-items: center; gap: .75rem; border-bottom: 2px solid var(--acento); padding-bottom: .5rem; }
header img { width: 44px; height: 44px; }
header .marca { font-weight: 600; color: var(--acento); }
nav { margin: .75rem 0 1.25rem; font-size: .95rem; }
a { color: var(--acento); }
h1 { font-size: 1.6rem; margin: 1rem 0 .5rem; }
h2 { font-size: 1.2rem; margin-top: 1.6rem; color: var(--acento); }
figure { background: var(--suave); border: 1px solid var(--borde); border-radius: 8px; margin: 1rem 0; padding: .6rem 1rem; }
figure math { font-size: 1.2rem; overflow-x: auto; padding: .4rem 0; }
figcaption { font-size: .95rem; }
table { border-collapse: collapse; width: 100%; margin: .5rem 0; }
th, td { border-bottom: 1px solid var(--borde); padding: .45rem .5rem; text-align: left; vertical-align: top; }
th { color: var(--acento); }
td:first-child { width: 34%; font-weight: 600; }
.nota { border-left: 4px solid var(--acento); padding: .3rem .8rem; background: var(--suave); }
ul.indice { columns: 2; padding-left: 1.2rem; }
footer { margin-top: 2.5rem; font-size: .85rem; opacity: .75; }
"""


def load_lang(lang):
    table = {}
    with open(os.path.join(ROOT, 'lang', f'{lang}.lang'), encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#') or '=' not in line:
                continue
            key, value = line.split('=', 1)
            table[key.strip()] = value.strip().replace('\\n', ' ')
    return table


def esc(text):
    return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')


def page_title(page, tr):
    if 'title' in page:
        return None
    return tr.get('op.' + page['id'], page['id'])


def all_pages():
    pages = list(pages_geometry.PAGES) + list(pages_filters.PAGES) + [pages_vector.PAGE]
    return {p['id']: p for p in pages}


def title_of(page, lang, tr):
    if 'title' in page:
        return page['title'][lang]
    return tr.get('op.' + page['id'], page['id']).rstrip('…')


def wrap(lang, title, body, tr):
    return f"""<!DOCTYPE html>
<html lang="{lang}">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>{esc(title)} · Lulumoy Studio</title>
<style>{CSS}</style>
</head>
<body>
<header><img src="../../assets/lulumoy_icon.png" alt=""/><span class="marca">Lulumoy Studio · {UI['guide'][lang]}</span></header>
{body}
<footer>Lulumoy Studio · DonatellaTech · <a href="https://donatellatech.com/">donatellatech.com</a></footer>
</body>
</html>
"""


def render_page(page, lang, tr, pages):
    title = title_of(page, lang, tr)
    out = [f'<nav><a href="index.html">{UI["back"][lang]}</a></nav>', f'<h1>{esc(title)}</h1>', f'<p>{page["intro"][lang]}</p>']
    for block in page['blocks']:
        kind = block[0]
        if kind == 'formula':
            out.append(f'<figure>{block[1]}<figcaption>{block[2][lang]}</figcaption></figure>')
        elif kind == 'text':
            out.append(f'<p>{block[1][lang]}</p>')
        elif kind == 'h':
            out.append(f'<h2>{block[1][lang]}</h2>')
    if page.get('params'):
        prefix = 'vec' if page['id'] == 'vectorize' else 'op.' + page['id']
        rows = []
        for key, desc in page['params'].items():
            label = tr.get(f'{prefix}.{key}')
            if label is None:
                raise SystemExit(f'Falta la traducción {prefix}.{key} en {lang}')
            rows.append(f'<tr><td>{esc(label)}</td><td>{desc[lang]}</td></tr>')
        out.append(f'<h2>{UI["params"][lang]}</h2><table><thead><tr><th>{UI["param"][lang]}</th><th>{UI["meaning"][lang]}</th></tr></thead><tbody>{"".join(rows)}</tbody></table>')
    if page.get('note'):
        out.append(f'<p class="nota"><b>{UI["note"][lang]}:</b> {page["note"][lang]}</p>')
    if page.get('see'):
        links = ', '.join(f'<a href="{s}.html">{esc(title_of(pages[s], lang, tr))}</a>' for s in page['see'])
        out.append(f'<p>{UI["see"][lang]}: {links}</p>')
    return wrap(lang, title, '\n'.join(out), tr)


def render_index(lang, tr, pages):
    out = [f'<h1>{UI["guide"][lang]}</h1>', f'<p>{UI["intro"][lang]}</p>']
    for group, ids in GROUPS:
        items = ''.join(f'<li><a href="{pid}.html">{esc(title_of(pages[pid], lang, tr))}</a></li>' for pid in ids)
        out.append(f'<h2>{UI[group][lang]}</h2><ul class="indice">{items}</ul>')
    out.append(f'<h2>{UI["notation"][lang]}</h2><p>{UI["notation_text"][lang]}</p>')
    return wrap(lang, UI['guide'][lang], '\n'.join(out), tr)


def main():
    pages = all_pages()
    count = 0
    for lang in LANGS:
        tr = load_lang(lang)
        folder = os.path.join(ROOT, 'help', lang)
        os.makedirs(folder, exist_ok=True)
        for pid, page in pages.items():
            with open(os.path.join(folder, pid + '.html'), 'w', encoding='utf-8') as f:
                f.write(render_page(page, lang, tr, pages))
            count += 1
        with open(os.path.join(folder, 'index.html'), 'w', encoding='utf-8') as f:
            f.write(render_index(lang, tr, pages))
        count += 1
    print(f'{count} páginas generadas en {ROOT}/help')


if __name__ == '__main__':
    main()
