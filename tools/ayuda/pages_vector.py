"""Página de la vectorización a SVG."""
from mathml import *


def L(es, en, it):
    return {'es': es, 'en': en, 'it': it}


T_, F_, C_, D_ = i('T'), i('F'), i('C'), i('D')
g_ = i('g')
p_, q_ = i('p'), i('q')
om0, om1 = sub(i('ω'), n(0)), sub(i('ω'), n(1))
mu0, mu1 = sub(i('μ'), n(0)), sub(i('μ'), n(1))
A_, B_, P_ = i('A'), i('B'), i('P')
V = lambda k_: sub(i('V'), k_)
Mi = lambda k_: sub(i('M'), k_)
iI = i('i')
s_ = i('s')
eps = i('ε')
th = sub(i('θ'), iI)
tv = i('t')

PAGE = {
    'id': 'vectorize',
    'title': L('Vectorización a SVG', 'SVG vectorization', 'Vettorializzazione in SVG'),
    'intro': L(
        'Convierte la imagen en trazados vectoriales en blanco y negro: primero decide qué píxeles forman parte de las formas (binarización), después sigue sus contornos y por último los simplifica y los suaviza. Los pasos, en el orden en que se aplican, son los siguientes.',
        'Turns the image into black-and-white vector paths: first it decides which pixels belong to the shapes (thresholding), then it follows their outlines, and finally it simplifies and smooths them. The steps, in the order they are applied, are as follows.',
        "Converte l'immagine in tracciati vettoriali in bianco e nero: prima decide quali pixel fanno parte delle forme (binarizzazione), poi ne segue i contorni e infine li semplifica e li ammorbidisce. I passi, nell'ordine in cui si applicano, sono i seguenti."),
    'blocks': [
        ('h', L('1. Resolución de trabajo y desenfoque', '1. Working resolution and blur', '1. Risoluzione di lavoro e sfocatura')),
        ('text', L(
            'La imagen se pasa a grises de 8 bits y se amplía s veces (s = 1, 2 o 4) con interpolación bicúbica. Trabajar a mayor resolución sitúa los bordes con precisión de fracción de píxel, lo que mejora mucho las imágenes pequeñas. Si W·H·s² superaría 4·10<sup>7</sup> píxeles, s se reduce automáticamente. Después se aplica un desenfoque gaussiano de σ·s, si σ &gt; 0. Todos los parámetros en píxeles se expresan en píxeles de la imagen original; internamente se multiplican por s.',
            'The image is converted to 8-bit grayscale and enlarged s times (s = 1, 2 or 4) with bicubic interpolation. Working at a higher resolution places edges with sub-pixel accuracy, which greatly improves small images. If W·H·s² would exceed 4·10<sup>7</sup> pixels, s is reduced automatically. Then a Gaussian blur of σ·s is applied, if σ &gt; 0. All parameters in pixels refer to pixels of the original image; internally they are multiplied by s.',
            "L'immagine si converte in grigi a 8 bit e si ingrandisce s volte (s = 1, 2 o 4) con interpolazione bicubica. Lavorare a risoluzione maggiore colloca i bordi con precisione sub-pixel, il che migliora molto le immagini piccole. Se W·H·s² superasse 4·10<sup>7</sup> pixel, s si riduce automaticamente. Poi si applica una sfocatura gaussiana di σ·s, se σ &gt; 0. Tutti i parametri in pixel si riferiscono ai pixel dell'immagine originale; internamente si moltiplicano per s.")),
        ('h', L('2. Binarización', '2. Thresholding', '2. Binarizzazione')),
        ('formula', MATH(F_, eq(), brace(p_, o(':'), call(i('I'), p_), o('≤'), call(T_, p_))),
         L('F es el conjunto de píxeles que se trazan (las formas oscuras). Con "Trazar las zonas claras" se usa el complementario.',
           'F is the set of pixels that are traced (the dark shapes). With "Trace light shapes" its complement is used.',
           'F è l’insieme dei pixel che si tracciano (le forme scure). Con "Traccia le zone chiare" si usa il complementare.')),
        ('text', L('<b>Fijo</b>: T es el mismo para toda la imagen, el nivel elegido (0–255).',
                   '<b>Fixed</b>: T is the same for the whole image, the chosen level (0–255).',
                   "<b>Fisso</b>: T è lo stesso per tutta l'immagine, il livello scelto (0–255).")),
        ('formula', MATH(sup(T_, o('∗')), eq(), under(iv('arg max'), T_), sp_('0.3em'), call(om0, T_), call(om1, T_), sup(brk(call(mu0, T_), o('−'), call(mu1, T_)), n(2))),
         L('<b>Automático (Otsu)</b>: elige el T que maximiza la varianza entre las dos clases del histograma (fondo y formas). ω<sub>0</sub>, ω<sub>1</sub> son las fracciones de píxeles a cada lado de T y μ<sub>0</sub>, μ<sub>1</sub> sus medias. Funciona bien cuando el histograma tiene dos picos claros.',
           '<b>Automatic (Otsu)</b>: chooses the T that maximises the between-class variance of the histogram (background and shapes). ω<sub>0</sub>, ω<sub>1</sub> are the fractions of pixels on each side of T and μ<sub>0</sub>, μ<sub>1</sub> their means. It works well when the histogram has two clear peaks.',
           "<b>Automatico (Otsu)</b>: sceglie il T che massimizza la varianza tra le due classi dell'istogramma (sfondo e forme). ω<sub>0</sub>, ω<sub>1</sub> sono le frazioni di pixel ai due lati di T e μ<sub>0</sub>, μ<sub>1</sub> le loro medie. Funziona bene quando l'istogramma ha due picchi netti.")),
        ('formula', MATH(call(T_, p_), eq(), under(o('∑'), m(q_, o('∈'), call(i('B'), p_))), call(g_, m(q_, o('−'), p_)), call(i('I'), q_), o('−'), C_),
         L('<b>Adaptativo</b>: T varía de un punto a otro; es la media local ponderada con una ventana gaussiana g de b×b píxeles, menos la constante C. Corrige la iluminación desigual y las sombras de los escaneos. Un píxel se traza si es al menos C niveles más oscuro que su entorno.',
           '<b>Adaptive</b>: T varies from point to point; it is the local mean weighted by a b×b Gaussian window g, minus the constant C. It corrects uneven lighting and scan shadows. A pixel is traced if it is at least C levels darker than its surroundings.',
           "<b>Adattivo</b>: T varia da punto a punto; è la media locale pesata con una finestra gaussiana g di b×b pixel, meno la costante C. Corregge l'illuminazione irregolare e le ombre delle scansioni. Un pixel si traccia se è almeno C livelli più scuro del suo intorno.")),
        ('h', L('3. Limpieza de bordes', '3. Edge cleaning', '3. Pulizia dei bordi')),
        ('formula', MATH(sup(F_, o('′')), eq(), par(F_, o('∘'), D_), o('•'), D_),
         L('Apertura y después cierre morfológicos con un disco D de diámetro 2r + 1 píxeles: la apertura elimina manchas y salientes de radio menor que r, y el cierre tapa huecos y grietas de ese tamaño.',
           'Morphological opening and then closing with a disc D of diameter 2r + 1 pixels: opening removes specks and protrusions with radius below r, and closing fills holes and gaps of that size.',
           'Apertura e poi chiusura morfologiche con un disco D di diametro 2r + 1 pixel: l’apertura elimina macchie e sporgenze di raggio minore di r, e la chiusura chiude buchi e fessure di quella dimensione.')),
        ('h', L('4. Contornos', '4. Outlines', '4. Contorni')),
        ('text', L(
            'cv::findContours sigue el borde de cada forma y de cada agujero (algoritmo de Suzuki y Abe). OpenCV devuelve los centros de los píxeles del borde, así que cada contorno se desplaza medio píxel hacia el fondo para que pase por el borde real de los píxeles. Sin este paso las formas adelgazarían un píxel y las líneas de 1 px desaparecerían.',
            'cv::findContours follows the border of every shape and every hole (Suzuki and Abe algorithm). OpenCV returns the centres of the border pixels, so each outline is shifted half a pixel towards the background to run along the actual pixel edges. Without this step shapes would be one pixel thinner and 1-px lines would vanish.',
            "cv::findContours segue il bordo di ogni forma e di ogni buco (algoritmo di Suzuki e Abe). OpenCV restituisce i centri dei pixel di bordo, quindi ogni contorno si sposta di mezzo pixel verso lo sfondo per passare sul bordo reale dei pixel. Senza questo passo le forme si assottiglierebbero di un pixel e le linee di 1 px sparirebbero.")),
        ('formula', MATH(i('A'), eq(), fr(n(1), n(2)), absv(underover(o('∑'), m(iI, eq(), n(1)), i('n')), par(sub(i('x'), iI), sub(i('y'), m(iI, o('+'), n(1))), o('−'), sub(i('x'), m(iI, o('+'), n(1))), sub(i('y'), iI)))),
         L('Área del contorno (fórmula del área de Gauss). Las formas y agujeros con A menor que "Ignorar manchas" se descartan; si se descarta una forma, también sus agujeros.',
           'Outline area (shoelace formula). Shapes and holes with A below "Ignore specks" are discarded; if a shape is discarded, so are its holes.',
           'Area del contorno (formula di Gauss). Le forme e i buchi con A minore di "Ignora macchie" si scartano; se si scarta una forma, anche i suoi buchi.')),
        ('h', L('5. Simplificación', '5. Simplification', '5. Semplificazione')),
        ('formula', MATH(call(i('d'), P_), eq(), fr(absv(par(B_, o('−'), A_), o('×'), par(P_, o('−'), A_)), norm(B_, o('−'), A_))),
         L('Algoritmo de Ramer–Douglas–Peucker (cv::approxPolyDP): entre dos vértices A y B, si el punto intermedio más alejado del segmento AB está a una distancia d &gt; ε, se conserva y se repite a cada lado; si no, todo el tramo se sustituye por el segmento. Ninguna parte del contorno simplificado se aleja más de ε del original. Si una forma fina quedara con menos de 3 vértices, ε se reduce a la mitad hasta que no ocurra.',
           'Ramer–Douglas–Peucker algorithm (cv::approxPolyDP): between two vertices A and B, if the intermediate point farthest from segment AB is at a distance d &gt; ε, it is kept and the process repeats on each side; otherwise the whole stretch is replaced by the segment. No part of the simplified outline is farther than ε from the original. If a thin shape would end up with fewer than 3 vertices, ε is halved until it does not.',
           "Algoritmo di Ramer–Douglas–Peucker (cv::approxPolyDP): tra due vertici A e B, se il punto intermedio più lontano dal segmento AB è a distanza d &gt; ε, si conserva e si ripete da ogni lato; altrimenti tutto il tratto si sostituisce con il segmento. Nessuna parte del contorno semplificato si allontana più di ε dall'originale. Se una forma sottile restasse con meno di 3 vertici, ε si dimezza finché non succede.")),
        ('h', L('6. Curvas y esquinas', '6. Curves and corners', '6. Curve e angoli')),
        ('formula', MATH(th, eq(), iv('arccos'), fr(m(par(V(iI), o('−'), V(m(iI, o('−'), n(1)))), o('·'), par(V(m(iI, o('+'), n(1))), o('−'), V(iI))),
                                                 m(norm(V(iI), o('−'), V(m(iI, o('−'), n(1)))), norm(V(m(iI, o('+'), n(1))), o('−'), V(iI))))),
         L('Ángulo de giro en cada vértice V<sub>i</sub> del polígono simplificado: 0° si el contorno sigue recto, 180° si vuelve sobre sí mismo.',
           'Turning angle at each vertex V<sub>i</sub> of the simplified polygon: 0° if the outline goes straight on, 180° if it doubles back.',
           'Angolo di svolta in ogni vertice V<sub>i</sub> del poligono semplificato: 0° se il contorno prosegue dritto, 180° se torna su sé stesso.')),
        ('formula', MATH(call(i('B'), tv), eq(), sup(par(n(1), o('−'), tv), n(2)), Mi(m(iI, o('−'), n(1))), o('+'), n(2), par(n(1), o('−'), tv), tv, V(iI), o('+'), sup(tv, n(2)), Mi(iI), comma(), sp_('1em'),
                         Mi(iI), eq(), fr(m(V(iI), o('+'), V(m(iI, o('+'), n(1)))), n(2)), comma(), sp_('1em'), tv, o('∈'), brk(n(0), comma(), n(1))),
         L('Con "Curvas suaves", el trazado pasa por los puntos medios M<sub>i</sub> de los lados y cada vértice V<sub>i</sub> con θ<sub>i</sub> ≤ θ<sub>esquina</sub> se usa como punto de control de una curva de Bézier cuadrática. En M<sub>i</sub> las dos curvas vecinas tienen la misma tangente (la dirección del lado), así que la unión es suave. Los vértices con θ<sub>i</sub> &gt; θ<sub>esquina</sub> se conservan como esquinas rectas.',
           'With "Smooth curves", the path passes through the midpoints M<sub>i</sub> of the sides and each vertex V<sub>i</sub> with θ<sub>i</sub> ≤ θ<sub>corner</sub> becomes the control point of a quadratic Bézier curve. At M<sub>i</sub> the two neighbouring curves share the same tangent (the direction of the side), so the joint is smooth. Vertices with θ<sub>i</sub> &gt; θ<sub>corner</sub> are kept as sharp corners.',
           'Con "Curve morbide", il tracciato passa per i punti medi M<sub>i</sub> dei lati e ogni vertice V<sub>i</sub> con θ<sub>i</sub> ≤ θ<sub>angolo</sub> diventa il punto di controllo di una curva di Bézier quadratica. In M<sub>i</sub> le due curve vicine hanno la stessa tangente (la direzione del lato), quindi la giunzione è morbida. I vertici con θ<sub>i</sub> &gt; θ<sub>angolo</sub> si conservano come angoli vivi.')),
        ('h', L('7. Archivo SVG', '7. SVG file', '7. File SVG')),
        ('text', L(
            'Todos los contornos forman un único trazado negro con la regla de relleno par-impar (fill-rule="evenodd"): un punto está relleno si una semirrecta que parte de él corta el trazado un número impar de veces, por eso los agujeros quedan transparentes. Las coordenadas están en píxeles de la imagen original, con el píxel (i, j) ocupando el cuadrado [i, i+1]×[j, j+1], y se escriben con dos decimales. Los "nodos" que muestra el diálogo son los tramos (rectas y curvas) del trazado.',
            'All outlines form a single black path with the even-odd fill rule (fill-rule="evenodd"): a point is filled if a ray starting from it crosses the path an odd number of times, which is why holes are transparent. Coordinates are in pixels of the original image, with pixel (i, j) covering the square [i, i+1]×[j, j+1], written with two decimals. The "nodes" shown in the dialog are the segments (lines and curves) of the path.',
            'Tutti i contorni formano un unico tracciato nero con la regola di riempimento pari-dispari (fill-rule="evenodd"): un punto è pieno se una semiretta che parte da esso taglia il tracciato un numero dispari di volte, per questo i buchi restano trasparenti. Le coordinate sono in pixel dell\'immagine originale, con il pixel (i, j) che occupa il quadrato [i, i+1]×[j, j+1], scritte con due decimali. I "nodi" mostrati nel dialogo sono i tratti (rette e curve) del tracciato.')),
    ],
    'params': {
        'method': L('Cómo se calcula T (paso 2).', 'How T is computed (step 2).', 'Come si calcola T (passo 2).'),
        'threshold': L('T del método fijo, de 0 a 255.', 'T for the fixed method, from 0 to 255.', 'T del metodo fisso, da 0 a 255.'),
        'block': L('b, lado de la ventana del método adaptativo (impar). Debe ser mayor que los trazos: unas 2–5 veces su grosor.',
                   'b, window side for the adaptive method (odd). It must be larger than the strokes: about 2–5 times their width.',
                   'b, lato della finestra del metodo adattivo (dispari). Deve essere più grande dei tratti: circa 2–5 volte il loro spessore.'),
        'offset': L('C, en niveles de gris. Más alto = solo se trazan los píxeles bastante más oscuros que su entorno (menos ruido).',
                    'C, in gray levels. Higher = only pixels clearly darker than their surroundings are traced (less noise).',
                    'C, in livelli di grigio. Più alto = si tracciano solo i pixel nettamente più scuri del loro intorno (meno rumore).'),
        'blur': L('σ del desenfoque previo (paso 1). Reduce el ruido y redondea los dientes de sierra.', 'σ of the pre-blur (step 1). Reduces noise and rounds jagged edges.', 'σ della sfocatura preliminare (passo 1). Riduce il rumore e arrotonda le seghettature.'),
        'invert': L('Traza las zonas claras en vez de las oscuras.', 'Traces light areas instead of dark ones.', 'Traccia le zone chiare invece di quelle scure.'),
        'scale': L('s (paso 1).', 's (step 1).', 's (passo 1).'),
        'clean': L('r del paso 3; 0 = sin limpieza.', 'r in step 3; 0 = no cleaning.', 'r del passo 3; 0 = nessuna pulizia.'),
        'min_area': L('Área mínima en px² (paso 4).', 'Minimum area in px² (step 4).', 'Area minima in px² (passo 4).'),
        'epsilon': L('ε del paso 5, en píxeles. 0 = sin simplificar: el SVG reproduce la imagen binarizada píxel a píxel.',
                     'ε in step 5, in pixels. 0 = no simplification: the SVG reproduces the thresholded image pixel by pixel.',
                     "ε del passo 5, in pixel. 0 = nessuna semplificazione: l'SVG riproduce l'immagine binarizzata pixel per pixel."),
        'curves': L('Polígonos o curvas de Bézier (paso 6).', 'Polygons or Bézier curves (step 6).', 'Poligoni o curve di Bézier (passo 6).'),
        'corner': L('θ<sub>esquina</sub> en grados (paso 6). 0 = todo esquinas (polígono); 180 = todo curvas.',
                    'θ<sub>corner</sub> in degrees (step 6). 0 = all corners (polygon); 180 = all curves.',
                    'θ<sub>angolo</sub> in gradi (passo 6). 0 = tutto angoli (poligono); 180 = tutto curve.'),
        'background': L('Añade un rectángulo blanco detrás del trazado; si no, el fondo es transparente.',
                        'Adds a white rectangle behind the path; otherwise the background is transparent.',
                        'Aggiunge un rettangolo bianco dietro il tracciato; altrimenti lo sfondo è trasparente.'),
    },
    'see': ['filter.morphology', 'interpolation'],
}
