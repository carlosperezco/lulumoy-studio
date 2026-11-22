"""Páginas de geometría: rotar, voltear, recortar, afín, perspectiva, interpolación."""
from mathml import *


def L(es, en, it):
    return {'es': es, 'en': en, 'it': it}


xp, yp = prime(x), prime(y)
th = i('θ')
cx, cy = sub(i('c'), i('x')), sub(i('c'), i('y'))
W, H = i('W'), i('H')

PAGES = []

# --------------------------------------------------------------------------- rotar
PAGES.append({
    'id': 'edit.rotate',
    'intro': L(
        'Gira la imagen un ángulo θ alrededor de su centro. Los ángulos positivos giran en sentido antihorario tal como se ve la imagen en pantalla (con el eje y hacia abajo).',
        'Rotates the image by an angle θ around its centre. Positive angles rotate counter-clockwise as the image is seen on screen (with the y axis pointing down).',
        "Ruota l'immagine di un angolo θ attorno al suo centro. Gli angoli positivi ruotano in senso antiorario come si vede l'immagine sullo schermo (con l'asse y verso il basso)."),
    'blocks': [
        ('formula', MATH(matrix([[xp], [yp]]), eq(),
                         matrix([[m(iv('cos'), th), m(iv('sin'), th), m(par(n(1), o('−'), iv('cos'), th), cx, o('−'), iv('sin'), th, cy)],
                                 [m(o('−'), iv('sin'), th), m(iv('cos'), th), m(iv('sin'), th, cx, o('+'), par(n(1), o('−'), iv('cos'), th), cy)]]),
                         matrix([[x], [y], [n(1)]])),
         L('Matriz de rotación de OpenCV (getRotationMatrix2D), con centro (c<sub>x</sub>, c<sub>y</sub>) = (W/2, H/2).',
           'OpenCV rotation matrix (getRotationMatrix2D), with centre (c<sub>x</sub>, c<sub>y</sub>) = (W/2, H/2).',
           'Matrice di rotazione di OpenCV (getRotationMatrix2D), con centro (c<sub>x</sub>, c<sub>y</sub>) = (W/2, H/2).')),
        ('formula', MATH(prime(W), eq(), ceil(absv(W, iv('cos'), th), o('+'), absv(H, iv('sin'), th)), comma(), sp_('1em'),
                         prime(H), eq(), ceil(absv(W, iv('sin'), th), o('+'), absv(H, iv('cos'), th))),
         L('Tamaño del lienzo cuando se amplía para que quepa toda la imagen; la traslación se corrige para que la imagen quede centrada.',
           'Canvas size when it is expanded to fit the whole image; the translation is adjusted so the image stays centred.',
           "Dimensione della tela quando viene ampliata per contenere tutta l'immagine; la traslazione si corregge perché l'immagine resti centrata.")),
        ('text', L(
            'Cada píxel del resultado se calcula con la transformación inversa y se interpola (bilineal); las zonas fuera de la imagen original quedan en negro. Los giros de 90°, 180° y 270° con lienzo ampliado se hacen reordenando píxeles, sin interpolación ni pérdida.',
            'Each output pixel is computed with the inverse transform and interpolated (bilinear); areas outside the original image are black. Rotations of 90°, 180° and 270° with an expanded canvas are done by reordering pixels, with no interpolation or loss.',
            "Ogni pixel del risultato si calcola con la trasformazione inversa e si interpola (bilineare); le zone fuori dall'immagine originale restano nere. Le rotazioni di 90°, 180° e 270° con tela ampliata si fanno riordinando i pixel, senza interpolazione né perdita.")),
    ],
    'params': {
        'angle': L('θ en grados, entre −360 y 360.', 'θ in degrees, between −360 and 360.', 'θ in gradi, tra −360 e 360.'),
        'expand': L('Si está marcada, el lienzo crece a W′×H′ para no recortar las esquinas; si no, conserva W×H.',
                    'If checked, the canvas grows to W′×H′ so no corner is cut; otherwise it keeps W×H.',
                    'Se selezionata, la tela cresce a W′×H′ per non tagliare gli angoli; altrimenti resta W×H.'),
    },
    'see': ['interpolation'],
})

# --------------------------------------------------------------------------- voltear
PAGES.append({
    'id': 'edit.flip',
    'intro': L('Refleja la imagen respecto a su eje vertical, horizontal o ambos (equivale a girar 180°).',
               'Mirrors the image about its vertical axis, horizontal axis, or both (equivalent to a 180° rotation).',
               "Riflette l'immagine rispetto al suo asse verticale, orizzontale o entrambi (equivale a ruotare di 180°)."),
    'blocks': [
        ('formula', MATH(Ipxy(), eq(), cases([
            (Ixy(m(W, o('−'), n(1), o('−'), x), y), t('Horizontal')),
            (Ixy(x, m(H, o('−'), n(1), o('−'), y)), t('Vertical')),
            (Ixy(m(W, o('−'), n(1), o('−'), x), m(H, o('−'), n(1), o('−'), y)), t('Ambas / Both / Entrambe')),
        ])), L('No hay interpolación: solo se reordenan píxeles.', 'There is no interpolation: pixels are only reordered.', "Non c'è interpolazione: si riordinano solo i pixel.")),
    ],
    'params': {
        'mode': L('Eje de reflexión.', 'Mirror axis.', 'Asse di riflessione.'),
    },
})

# --------------------------------------------------------------------------- recortar
x0, y0 = sub(x, n(0)), sub(y, n(0))
w_, h_ = i('w'), i('h')
PAGES.append({
    'id': 'edit.crop',
    'intro': L('Conserva un rectángulo de la imagen.', 'Keeps a rectangle of the image.', "Conserva un rettangolo dell'immagine."),
    'blocks': [
        ('formula', MATH(Ipxy(), eq(), Ixy(m(x, o('+'), x0), m(y, o('+'), y0)), comma(), sp_('1em'),
                         n(0), o('≤'), x, o('&lt;'), w_, comma(), sp_('0.5em'), n(0), o('≤'), y, o('&lt;'), h_),
         L('Si el ancho o el alto valen 0, o el rectángulo sale de la imagen, se recorta hasta el borde: w = W − x<sub>0</sub>, h = H − y<sub>0</sub>.',
           'If width or height is 0, or the rectangle goes past the image, it is cut at the edge: w = W − x<sub>0</sub>, h = H − y<sub>0</sub>.',
           "Se larghezza o altezza valgono 0, o il rettangolo esce dall'immagine, si taglia al bordo: w = W − x<sub>0</sub>, h = H − y<sub>0</sub>.")),
    ],
    'params': {
        'x': L('x<sub>0</sub>: columna del borde izquierdo.', 'x<sub>0</sub>: column of the left edge.', 'x<sub>0</sub>: colonna del bordo sinistro.'),
        'y': L('y<sub>0</sub>: fila del borde superior.', 'y<sub>0</sub>: row of the top edge.', 'y<sub>0</sub>: riga del bordo superiore.'),
        'width': L('w en píxeles.', 'w in pixels.', 'w in pixel.'),
        'height': L('h en píxeles.', 'h in pixels.', 'h in pixel.'),
    },
})

# --------------------------------------------------------------------------- afín
a = lambda r, c: sub(i('a'), n(f'{r}{c}'))
tt = lambda r: sub(i('t'), n(r))
xk, yk = sub(x, i('k')), sub(y, i('k'))
xpk, ypk = sub(xp, i('k')) if False else subsup(x, i('k'), o('′')), subsup(y, i('k'), o('′'))
PAGES.append({
    'id': 'edit.affine',
    'intro': L(
        'Transformación lineal más traslación. Conserva las rectas, el paralelismo y las proporciones a lo largo de una recta, pero no los ángulos ni las distancias. Incluye como casos particulares la traslación, la rotación, el escalado y el cizallamiento.',
        'A linear map plus a translation. It preserves straight lines, parallelism and ratios along a line, but not angles or distances. Translation, rotation, scaling and shear are particular cases.',
        "Trasformazione lineare più traslazione. Conserva le rette, il parallelismo e le proporzioni lungo una retta, ma non gli angoli né le distanze. Traslazione, rotazione, scala e taglio ne sono casi particolari."),
    'blocks': [
        ('formula', MATH(matrix([[xp], [yp]]), eq(), matrix([[a(1, 1), a(1, 2)], [a(2, 1), a(2, 2)]]), matrix([[x], [y]]), o('+'), matrix([[tt(1)], [tt(2)]])),
         L('Seis incógnitas. Cada par de puntos (x<sub>k</sub>, y<sub>k</sub>) → (x′<sub>k</sub>, y′<sub>k</sub>) aporta dos ecuaciones lineales, así que tres pares determinan la transformación (cv::getAffineTransform resuelve el sistema 6×6).',
           'Six unknowns. Each point pair (x<sub>k</sub>, y<sub>k</sub>) → (x′<sub>k</sub>, y′<sub>k</sub>) gives two linear equations, so three pairs determine the transform (cv::getAffineTransform solves the 6×6 system).',
           'Sei incognite. Ogni coppia di punti (x<sub>k</sub>, y<sub>k</sub>) → (x′<sub>k</sub>, y′<sub>k</sub>) dà due equazioni lineari, quindi tre coppie determinano la trasformazione (cv::getAffineTransform risolve il sistema 6×6).')),
        ('formula', MATH(par(sub(x, n(2)), o('−'), sub(x, n(1))), par(sub(y, n(3)), o('−'), sub(y, n(1))), o('−'),
                         par(sub(x, n(3)), o('−'), sub(x, n(1))), par(sub(y, n(2)), o('−'), sub(y, n(1))), o('≠'), n(0)),
         L('El sistema tiene solución única si los tres puntos no están alineados (área del triángulo distinta de cero). El programa exige al menos 0,5 px² de área en el origen y en el destino.',
           'The system has a unique solution if the three points are not collinear (non-zero triangle area). The program requires at least 0.5 px² of area at source and destination.',
           "Il sistema ha soluzione unica se i tre punti non sono allineati (area del triangolo diversa da zero). Il programma richiede almeno 0,5 px² di area all'origine e alla destinazione.")),
        ('formula', MATH(Ipxy(xp, yp), eq(), call(I, m(sup(i('A'), m(o('−'), n(1))), par(matrix([[xp], [yp]]), o('−'), i('t'))))),
         L('cv::warpAffine recorre la imagen de destino y busca cada píxel en el origen con la transformación inversa; como cae entre píxeles, se interpola.',
           'cv::warpAffine walks the destination image and looks up each pixel in the source with the inverse transform; since it falls between pixels, it is interpolated.',
           "cv::warpAffine percorre l'immagine di destinazione e cerca ogni pixel nell'origine con la trasformazione inversa; poiché cade tra i pixel, si interpola.")),
        ('text', L('Los puntos se eligen con el ratón: para cada par, primero el origen (amarillo) y luego el destino (cian). El resultado conserva el tamaño de la imagen.',
                   'Points are chosen with the mouse: for each pair, first the source (yellow) and then the destination (cyan). The result keeps the image size.',
                   "I punti si scelgono con il mouse: per ogni coppia, prima l'origine (giallo) e poi la destinazione (ciano). Il risultato mantiene la dimensione dell'immagine.")),
    ],
    'params': {
        'interpolation': L('Cómo se calcula el valor en posiciones no enteras.', 'How values at non-integer positions are computed.', 'Come si calcola il valore in posizioni non intere.'),
        'border': L('Qué valor toman los píxeles que caen fuera de la imagen original.', 'Value of pixels that fall outside the original image.', "Valore dei pixel che cadono fuori dall'immagine originale."),
    },
    'see': ['interpolation', 'edit.perspective'],
})

# --------------------------------------------------------------------------- perspectiva
h = lambda r, c: sub(i('h'), n(f'{r}{c}'))
u_, v_, w2 = i('u'), i('v'), i('w')
PAGES.append({
    'id': 'edit.perspective',
    'intro': L(
        'Homografía: la transformación entre dos vistas de un plano tomadas desde puntos distintos. Conserva las rectas, pero no el paralelismo (las paralelas pueden converger en un punto de fuga). Sirve para enderezar documentos, fachadas o pizarras fotografiados en ángulo.',
        'Homography: the mapping between two views of a plane taken from different points. It preserves straight lines but not parallelism (parallel lines may converge to a vanishing point). Useful to straighten documents, façades or boards photographed at an angle.',
        "Omografia: la trasformazione tra due viste di un piano riprese da punti diversi. Conserva le rette ma non il parallelismo (le parallele possono convergere in un punto di fuga). Serve a raddrizzare documenti, facciate o lavagne fotografati di sbieco."),
    'blocks': [
        ('formula', MATH(matrix([[u_], [v_], [w2]]), eq(), matrix([[h(1, 1), h(1, 2), h(1, 3)], [h(2, 1), h(2, 2), h(2, 3)], [h(3, 1), h(3, 2), n(1)]]),
                         matrix([[x], [y], [n(1)]]), comma(), sp_('1.5em'), xp, eq(), fr(u_, w2), comma(), sp_('1em'), yp, eq(), fr(v_, w2)),
         L('Coordenadas homogéneas: la división entre w produce el efecto de perspectiva. H se define salvo un factor de escala, por eso h<sub>33</sub> = 1 y quedan 8 incógnitas.',
           'Homogeneous coordinates: dividing by w produces the perspective effect. H is defined up to a scale factor, hence h<sub>33</sub> = 1 and 8 unknowns remain.',
           "Coordinate omogenee: la divisione per w produce l'effetto prospettico. H è definita a meno di un fattore di scala, perciò h<sub>33</sub> = 1 e restano 8 incognite.")),
        ('formula', MATH(cases([
            (m(xpk, par(h(3, 1), xk, o('+'), h(3, 2), yk, o('+'), n(1)), eq(), h(1, 1), xk, o('+'), h(1, 2), yk, o('+'), h(1, 3)), t('')),
            (m(ypk, par(h(3, 1), xk, o('+'), h(3, 2), yk, o('+'), n(1)), eq(), h(2, 1), xk, o('+'), h(2, 2), yk, o('+'), h(2, 3)), m(i('k'), eq(), n(1), comma(), o('…'), comma(), n(4))),
        ])),
         L('Cuatro pares de puntos dan ocho ecuaciones lineales en las h<sub>ij</sub>; cv::getPerspectiveTransform resuelve el sistema 8×8. Tiene solución única si no hay tres puntos alineados ni en el origen ni en el destino, que es lo que comprueba el programa.',
           'Four point pairs give eight linear equations in the h<sub>ij</sub>; cv::getPerspectiveTransform solves the 8×8 system. The solution is unique if no three points are collinear at source or destination, which the program checks.',
           "Quattro coppie di punti danno otto equazioni lineari nelle h<sub>ij</sub>; cv::getPerspectiveTransform risolve il sistema 8×8. La soluzione è unica se non ci sono tre punti allineati né all'origine né alla destinazione, ed è ciò che controlla il programma.")),
        ('text', L('Como en la afín, cv::warpPerspective calcula cada píxel del destino con la transformación inversa H<sup>−1</sup> y lo interpola. El resultado conserva el tamaño de la imagen.',
                   'As with the affine transform, cv::warpPerspective computes each destination pixel with the inverse transform H<sup>−1</sup> and interpolates it. The result keeps the image size.',
                   "Come per l'affine, cv::warpPerspective calcola ogni pixel di destinazione con la trasformazione inversa H<sup>−1</sup> e lo interpola. Il risultato mantiene la dimensione dell'immagine.")),
    ],
    'params': {
        'interpolation': L('Cómo se calcula el valor en posiciones no enteras.', 'How values at non-integer positions are computed.', 'Come si calcola il valore in posizioni non intere.'),
        'border': L('Qué valor toman los píxeles que caen fuera de la imagen original.', 'Value of pixels that fall outside the original image.', "Valore dei pixel che cadono fuori dall'immagine originale."),
    },
    'see': ['interpolation', 'edit.affine'],
})

# --------------------------------------------------------------------------- interpolación
ax_, by_ = i('a'), i('b')
I00, I10, I01, I11 = sub(I, n('00')), sub(I, n('10')), sub(I, n('01')), sub(I, n('11'))
tv = i('t')
PAGES.append({
    'id': 'interpolation',
    'title': L('Interpolación y bordes', 'Interpolation and borders', 'Interpolazione e bordi'),
    'intro': L(
        'Las transformaciones geométricas buscan cada píxel del resultado en una posición (x, y) de la imagen original que casi nunca es entera. La interpolación estima el valor en esa posición a partir de los píxeles vecinos.',
        'Geometric transforms look up each output pixel at a position (x, y) of the original image that is almost never an integer. Interpolation estimates the value there from the neighbouring pixels.',
        "Le trasformazioni geometriche cercano ogni pixel del risultato in una posizione (x, y) dell'immagine originale che quasi mai è intera. L'interpolazione stima il valore in quella posizione a partire dai pixel vicini."),
    'blocks': [
        ('h', L('Vecino más cercano', 'Nearest neighbour', 'Vicino più prossimo')),
        ('formula', MATH(Ixy(), o('≈'), call(I, m(iv('round'), par(x)), m(iv('round'), par(y)))),
         L('Rápida y sin valores nuevos (útil en imágenes binarias), pero produce escalones.',
           'Fast and creates no new values (useful for binary images), but produces jagged edges.',
           'Veloce e senza valori nuovi (utile per immagini binarie), ma produce scalini.')),
        ('h', L('Lineal (bilineal)', 'Linear (bilinear)', 'Lineare (bilineare)')),
        ('formula', MATH(Ixy(), o('≈'), par(n(1), o('−'), ax_), par(n(1), o('−'), by_), I00, o('+'), ax_, par(n(1), o('−'), by_), I10, o('+'),
                         par(n(1), o('−'), ax_), by_, I01, o('+'), ax_, by_, I11),
         L('a = x − ⌊x⌋, b = y − ⌊y⌋; I<sub>00</sub>…I<sub>11</sub> son los cuatro píxeles que rodean el punto. Es la opción por defecto: suave y rápida.',
           'a = x − ⌊x⌋, b = y − ⌊y⌋; I<sub>00</sub>…I<sub>11</sub> are the four pixels around the point. It is the default: smooth and fast.',
           'a = x − ⌊x⌋, b = y − ⌊y⌋; I<sub>00</sub>…I<sub>11</sub> sono i quattro pixel attorno al punto. È la scelta predefinita: morbida e veloce.')),
        ('h', L('Cúbica (bicúbica)', 'Cubic (bicubic)', 'Cubica (bicubica)')),
        ('formula', MATH(call(i('w'), tv), eq(), cases([
            (m(par(ax_, o('+'), n(2)), sup(absv(tv), n(3)), o('−'), par(ax_, o('+'), n(3)), sup(absv(tv), n(2)), o('+'), n(1)), m(absv(tv), o('≤'), n(1))),
            (m(ax_, sup(absv(tv), n(3)), o('−'), n(5), ax_, sup(absv(tv), n(2)), o('+'), n(8), ax_, absv(tv), o('−'), n(4), ax_), m(n(1), o('&lt;'), absv(tv), o('&lt;'), n(2))),
            (n(0), t('en otro caso / otherwise / altrimenti')),
        ]), comma(), sp_('1em'), ax_, eq(), o('−'), n('0.75')),
         L('Núcleo cúbico de Keys con a = −0,75 (el de OpenCV), aplicado sobre los 4×4 píxeles vecinos: I ≈ Σ<sub>i,j</sub> w(x − x<sub>i</sub>) w(y − y<sub>j</sub>) I<sub>ij</sub>. Más nítida que la bilineal, aunque puede crear pequeños halos junto a los bordes.',
           'Keys cubic kernel with a = −0.75 (the one OpenCV uses), applied over the 4×4 neighbouring pixels: I ≈ Σ<sub>i,j</sub> w(x − x<sub>i</sub>) w(y − y<sub>j</sub>) I<sub>ij</sub>. Sharper than bilinear, though it may create small halos next to edges.',
           'Nucleo cubico di Keys con a = −0,75 (quello di OpenCV), applicato ai 4×4 pixel vicini: I ≈ Σ<sub>i,j</sub> w(x − x<sub>i</sub>) w(y − y<sub>j</sub>) I<sub>ij</sub>. Più nitida della bilineare, ma può creare piccoli aloni vicino ai bordi.')),
        ('h', L('Zonas vacías (bordes)', 'Empty areas (borders)', 'Aree vuote (bordi)')),
        ('text', L(
            'Cuando la posición buscada cae fuera de la imagen original: <b>Negro</b> usa 0; <b>Repetir el borde</b> usa el píxel más cercano del borde (aaa|abcd|ddd); <b>Espejo</b> refleja la imagen en el borde (cba|abcd|dcb).',
            'When the position falls outside the original image: <b>Black</b> uses 0; <b>Repeat edge</b> uses the nearest edge pixel (aaa|abcd|ddd); <b>Mirror</b> reflects the image at the edge (cba|abcd|dcb).',
            "Quando la posizione cade fuori dall'immagine originale: <b>Nero</b> usa 0; <b>Ripeti il bordo</b> usa il pixel di bordo più vicino (aaa|abcd|ddd); <b>Specchio</b> riflette l'immagine al bordo (cba|abcd|dcb).")),
    ],
    'params': {},
    'see': ['edit.rotate', 'edit.affine', 'edit.perspective'],
})
