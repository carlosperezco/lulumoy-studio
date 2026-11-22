"""Páginas de filtros y de formato."""
from mathml import *


def L(es, en, it):
    return {'es': es, 'en': en, 'it': it}


PAGES = []
ii, jj = i('i'), i('j')
p_, q_ = i('p'), i('q')
alpha, beta = i('α'), i('β')
T_ = i('T')
K_ = i('K')
a_ = i('a')

r_def = MATH(r, eq(), fr(m(k, o('−'), n(1)), n(2)))

# --------------------------------------------------------------------------- gaussiano
PAGES.append({
    'id': 'filter.gaussian',
    'intro': L('Promedio ponderado de los vecinos con una campana de Gauss: atenúa el ruido y los detalles finos, más cerca del centro pesa más.',
               'Weighted average of the neighbours using a Gaussian bell: it attenuates noise and fine detail, giving more weight near the centre.',
               'Media pesata dei vicini con una campana di Gauss: attenua il rumore e i dettagli fini, dando più peso vicino al centro.'),
    'blocks': [
        ('formula', MATH(call(i('G'), x, y), eq(), fr(n(1), m(n(2), i('π'), sup(sigma, n(2)))), sup(i('e'), m(o('−'), fr(m(sup(x, n(2)), o('+'), sup(y, n(2))), m(n(2), sup(sigma, n(2))))))),
         L('Núcleo gaussiano bidimensional.', 'Two-dimensional Gaussian kernel.', 'Nucleo gaussiano bidimensionale.')),
        ('formula', MATH(Ipxy(), eq(), sum2(m(o('−'), r), r), call(i('G'), ii, jj), times(), Ixy(m(x, o('+'), ii), m(y, o('+'), jj)), comma(), sp_('1em'), r, eq(), fr(m(k, o('−'), n(1)), n(2))),
         L('El núcleo se trunca a k×k y se normaliza para que sus pesos sumen 1, así el brillo medio no cambia. Es separable: OpenCV lo aplica como dos filtros de una dimensión.',
           'The kernel is truncated to k×k and normalised so its weights add up to 1, so the mean brightness does not change. It is separable: OpenCV applies it as two one-dimensional filters.',
           'Il nucleo si tronca a k×k e si normalizza perché i pesi sommino 1, così la luminosità media non cambia. È separabile: OpenCV lo applica come due filtri monodimensionali.')),
        ('formula', MATH(sigma, eq(), n('0.3'), par(fr(m(k, o('−'), n(1)), n(2)), o('−'), n(1)), o('+'), n('0.8')),
         L('Sigma que usa OpenCV cuando se deja en 0 (automático).', 'Sigma used by OpenCV when it is left at 0 (automatic).', 'Sigma usato da OpenCV quando si lascia a 0 (automatico).')),
    ],
    'params': {
        'ksize': L('k, lado del núcleo. Debe ser impar: un valor par se sube al impar siguiente. Con sigma fijo, conviene k ≈ 6σ + 1 para no truncar la campana.',
                   'k, side of the kernel. It must be odd: an even value is raised to the next odd one. With a fixed sigma, k ≈ 6σ + 1 avoids truncating the bell.',
                   'k, lato del nucleo. Deve essere dispari: un valore pari si porta al dispari successivo. Con sigma fisso, conviene k ≈ 6σ + 1 per non troncare la campana.'),
        'sigma': L('σ en píxeles: cuanto mayor, más desenfoque. 0 = calculado a partir de k.',
                   'σ in pixels: the larger, the stronger the blur. 0 = computed from k.',
                   'σ in pixel: più è grande, più sfocatura. 0 = calcolato a partire da k.'),
    },
})

# --------------------------------------------------------------------------- media
PAGES.append({
    'id': 'filter.box',
    'intro': L('Media aritmética de la ventana: todos los vecinos pesan lo mismo.', 'Arithmetic mean of the window: all neighbours weigh the same.', 'Media aritmetica della finestra: tutti i vicini pesano uguale.'),
    'blocks': [
        ('formula', MATH(Ipxy(), eq(), fr(n(1), sup(k, n(2))), sum2(m(o('−'), r), r), Ixy(m(x, o('+'), ii), m(y, o('+'), jj))),
         L('Para k impar, r = (k − 1)/2. Es el filtro más barato, pero su respuesta en frecuencia tiene lóbulos secundarios y puede dejar artefactos en forma de cuadrícula.',
           'For odd k, r = (k − 1)/2. It is the cheapest filter, but its frequency response has side lobes and may leave grid-like artefacts.',
           'Per k dispari, r = (k − 1)/2. È il filtro più economico, ma la sua risposta in frequenza ha lobi secondari e può lasciare artefatti a griglia.')),
    ],
    'params': {
        'ksize': L('k, lado de la ventana en píxeles.', 'k, side of the window in pixels.', 'k, lato della finestra in pixel.'),
    },
})

# --------------------------------------------------------------------------- mediana
PAGES.append({
    'id': 'filter.median',
    'intro': L('Sustituye cada píxel por la mediana de su ventana. No es lineal: elimina el ruido impulsivo ("sal y pimienta") conservando mejor los bordes que un promedio.',
               'Replaces each pixel by the median of its window. It is non-linear: it removes impulse ("salt and pepper") noise while preserving edges better than an average.',
               'Sostituisce ogni pixel con la mediana della sua finestra. Non è lineare: elimina il rumore impulsivo ("sale e pepe") conservando i bordi meglio di una media.'),
    'blocks': [
        ('formula', MATH(Ipxy(), eq(), iv('med'), brace(Ixy(m(x, o('+'), ii), m(y, o('+'), jj)), o(':'), absv(ii), comma(), absv(jj), o('≤'), r)),
         L('La mediana es el valor central tras ordenar los k² valores. En color se calcula por canal.',
           'The median is the middle value after sorting the k² values. In colour images it is computed per channel.',
           'La mediana è il valore centrale dopo aver ordinato i k² valori. A colori si calcola per canale.')),
    ],
    'params': {
        'ksize': L('k, impar y mayor o igual que 3. OpenCV solo admite k > 5 en imágenes de 8 bits.',
                   'k, odd and at least 3. OpenCV only allows k > 5 on 8-bit images.',
                   'k, dispari e almeno 3. OpenCV ammette k > 5 solo su immagini a 8 bit.'),
    },
})

# --------------------------------------------------------------------------- bilateral
Gs = sub(i('G'), sub(sigma, i('s')))
Gc = sub(i('G'), sub(sigma, i('c')))
Wp = sub(i('W'), p_)
PAGES.append({
    'id': 'filter.bilateral',
    'intro': L('Suaviza las zonas uniformes sin difuminar los bordes: cada vecino pesa según su distancia y según cuánto se parece su valor al del píxel central.',
               'Smooths uniform areas without blurring edges: each neighbour is weighted by its distance and by how similar its value is to the centre pixel.',
               'Leviga le zone uniformi senza sfumare i bordi: ogni vicino pesa secondo la distanza e secondo quanto il suo valore somiglia a quello del pixel centrale.'),
    'blocks': [
        ('formula', MATH(call(Ip, p_), eq(), fr(n(1), Wp), under(o('∑'), m(q_, o('∈'), call(i('S'), p_))), call(Gs, norm(p_, o('−'), q_)), times(),
                         call(Gc, absv(call(I, p_), o('−'), call(I, q_))), times(), call(I, q_)),
         L('S(p) es el disco de diámetro d centrado en p y W<sub>p</sub> la suma de los pesos, para normalizar.',
           'S(p) is the disc of diameter d centred at p and W<sub>p</sub> is the sum of the weights, for normalisation.',
           'S(p) è il disco di diametro d centrato in p e W<sub>p</sub> la somma dei pesi, per normalizzare.')),
        ('formula', MATH(call(sub(i('G'), sigma), i('t')), eq(), sup(i('e'), m(o('−'), fr(sup(i('t'), n(2)), m(n(2), sup(sigma, n(2))))))),
         L('En color, |I(p) − I(q)| es la suma de las diferencias absolutas de los tres canales.',
           'In colour images, |I(p) − I(q)| is the sum of the absolute differences of the three channels.',
           'A colori, |I(p) − I(q)| è la somma delle differenze assolute dei tre canali.')),
    ],
    'params': {
        'd': L('d, diámetro de la vecindad en píxeles. Valores grandes (más de 9) son lentos.', 'd, neighbourhood diameter in pixels. Large values (above 9) are slow.', "d, diametro dell'intorno in pixel. Valori grandi (oltre 9) sono lenti."),
        'sigma_color': L('σ<sub>c</sub>: diferencia de intensidad a partir de la cual dos píxeles "no se parecen". Grande = se comporta como un gaussiano.',
                         'σ<sub>c</sub>: intensity difference beyond which two pixels "do not look alike". Large = behaves like a Gaussian.',
                         'σ<sub>c</sub>: differenza di intensità oltre la quale due pixel "non si somigliano". Grande = si comporta come un gaussiano.'),
        'sigma_space': L('σ<sub>s</sub>: alcance espacial del promedio, en píxeles.', 'σ<sub>s</sub>: spatial reach of the average, in pixels.', 'σ<sub>s</sub>: portata spaziale della media, in pixel.'),
    },
    'note': L('Solo imágenes de 8 bits, en grises o color sin canal alfa.', 'Only 8-bit images, grayscale or colour without alpha.', 'Solo immagini a 8 bit, in grigi o a colori senza canale alfa.'),
})

# --------------------------------------------------------------------------- enfocar
PAGES.append({
    'id': 'filter.sharpen',
    'intro': L('Máscara de desenfoque (unsharp mask): resta a la imagen una versión desenfocada para extraer los detalles y los vuelve a sumar amplificados.',
               'Unsharp mask: subtracts a blurred version from the image to extract the detail and adds it back amplified.',
               "Maschera di contrasto (unsharp mask): sottrae all'immagine una versione sfocata per estrarre i dettagli e li risomma amplificati."),
    'blocks': [
        ('formula', MATH(Ip, eq(), I, o('+'), a_, par(I, o('−'), sub(i('G'), sigma), o('∗'), I), eq(), par(n(1), o('+'), a_), I, o('−'), a_, par(sub(i('G'), sigma), o('∗'), I)),
         L('G<sub>σ</sub> ∗ I es el desenfoque gaussiano. I − G<sub>σ</sub> ∗ I es un filtro paso alto: los bordes y texturas de tamaño menor que unos σ píxeles.',
           'G<sub>σ</sub> ∗ I is the Gaussian blur. I − G<sub>σ</sub> ∗ I is a high-pass filter: edges and textures smaller than a few σ pixels.',
           'G<sub>σ</sub> ∗ I è la sfocatura gaussiana. I − G<sub>σ</sub> ∗ I è un filtro passa-alto: bordi e trame più piccoli di qualche σ pixel.')),
    ],
    'params': {
        'amount': L('a, ganancia del detalle. 0 = sin cambio; valores altos crean halos.', 'a, detail gain. 0 = no change; high values create halos.', 'a, guadagno del dettaglio. 0 = nessun cambio; valori alti creano aloni.'),
        'sigma': L('σ del desenfoque: escala de los detalles que se refuerzan.', 'Blur σ: scale of the details that are boosted.', 'σ della sfocatura: scala dei dettagli che si rinforzano.'),
    },
})

# --------------------------------------------------------------------------- Canny
Gx, Gy = sub(i('G'), x), sub(i('G'), y)
Sx, Sy = sub(i('S'), x), sub(i('S'), y)
T1, T2 = sub(i('T'), n(1)), sub(i('T'), n(2))
grad = absv(o('∇'), I)
PAGES.append({
    'id': 'filter.canny',
    'intro': L('Detector de bordes de Canny: busca las líneas de máximo gradiente y decide cuáles son bordes con dos umbrales. El resultado es una imagen binaria (0 o 255) en grises.',
               'Canny edge detector: finds the lines of maximum gradient and decides which are edges with two thresholds. The result is a binary (0 or 255) grayscale image.',
               "Rivelatore di bordi di Canny: cerca le linee di massimo gradiente e decide quali sono bordi con due soglie. Il risultato è un'immagine binaria (0 o 255) in grigi."),
    'blocks': [
        ('formula', MATH(Gx, eq(), Sx, o('∗'), I, comma(), sp_('0.8em'), Gy, eq(), Sy, o('∗'), I, comma(), sp_('0.8em'),
                         grad, eq(), absv(Gx), o('+'), absv(Gy), comma(), sp_('0.8em'), i('θ'), eq(), iv('atan2'), par(Gy, comma(), Gx)),
         L('S<sub>x</sub>, S<sub>y</sub> son los operadores de Sobel del tamaño elegido. OpenCV usa por defecto la norma L1 del gradiente.',
           'S<sub>x</sub>, S<sub>y</sub> are the Sobel operators of the chosen size. OpenCV uses the L1 norm of the gradient by default.',
           'S<sub>x</sub>, S<sub>y</sub> sono gli operatori di Sobel della dimensione scelta. OpenCV usa per default la norma L1 del gradiente.')),
        ('text', L('Supresión de no máximos: un píxel solo sigue siendo candidato si su |∇I| es máximo en la dirección θ, lo que adelgaza los bordes a un píxel.',
                   'Non-maximum suppression: a pixel remains a candidate only if its |∇I| is a maximum along direction θ, which thins edges to one pixel.',
                   'Soppressione dei non massimi: un pixel resta candidato solo se il suo |∇I| è massimo nella direzione θ, il che assottiglia i bordi a un pixel.')),
        ('formula', MATH(t('borde / edge / bordo'), o('⟺'), cases([
            (m(grad, o('&gt;'), T2), t('')),
            (m(T1, o('&lt;'), grad, o('≤'), T2), t('y conectado a un borde fuerte / and connected to a strong edge / e connesso a un bordo forte')),
        ])), L('Histéresis: los píxeles por debajo de T<sub>1</sub> se descartan.', 'Hysteresis: pixels below T<sub>1</sub> are discarded.', 'Isteresi: i pixel sotto T<sub>1</sub> si scartano.')),
    ],
    'params': {
        'threshold1': L('T<sub>1</sub>, umbral bajo.', 'T<sub>1</sub>, low threshold.', 'T<sub>1</sub>, soglia bassa.'),
        'threshold2': L('T<sub>2</sub>, umbral alto. Una relación T<sub>2</sub>/T<sub>1</sub> entre 2 y 3 suele funcionar bien.',
                        'T<sub>2</sub>, high threshold. A ratio T<sub>2</sub>/T<sub>1</sub> between 2 and 3 usually works well.',
                        'T<sub>2</sub>, soglia alta. Un rapporto T<sub>2</sub>/T<sub>1</sub> tra 2 e 3 di solito funziona bene.'),
        'aperture': L('Tamaño del operador de Sobel (3, 5 o 7). Aperturas mayores dan gradientes más grandes, así que los umbrales deben subir en proporción.',
                      'Size of the Sobel operator (3, 5 or 7). Larger apertures give larger gradients, so thresholds must rise accordingly.',
                      "Dimensione dell'operatore di Sobel (3, 5 o 7). Aperture maggiori danno gradienti più grandi, quindi le soglie devono salire in proporzione."),
    },
    'note': L('La imagen se convierte antes a grises de 8 bits.', 'The image is first converted to 8-bit grayscale.', "L'immagine si converte prima in grigi a 8 bit."),
})

# --------------------------------------------------------------------------- umbral
PAGES.append({
    'id': 'filter.threshold',
    'intro': L('Compara cada píxel con un umbral T. El nivel se da en porcentaje para que funcione igual en 8 y en 16 bits.',
               'Compares each pixel with a threshold T. The level is given as a percentage so it works the same in 8 and 16 bits.',
               'Confronta ogni pixel con una soglia T. Il livello si dà in percentuale perché funzioni allo stesso modo a 8 e a 16 bit.'),
    'blocks': [
        ('formula', MATH(T_, eq(), fr(i('p'), n(100)), times(), M, comma(), sp_('1em'), M, o('∈'), brace(n(255), comma(), n(65535))),
         L('p es el nivel en % y M el valor máximo de la profundidad.', 'p is the level in % and M the maximum value for the bit depth.', 'p è il livello in % e M il valore massimo della profondità.')),
        ('formula', MATH(Ip, eq(), cases([
            (m(M, t(' si / if / se '), I, o('&gt;'), T_, comma(), t(' 0 en otro caso')), t('Binario / Binary')),
            (m(n(0), t(' si / if / se '), I, o('&gt;'), T_, comma(), t(' M en otro caso')), t('Binario invertido / Binary inverted')),
            (m(iv('min'), par(I, comma(), T_)), t('Truncar / Truncate')),
            (m(I, t(' si / if / se '), I, o('&gt;'), T_, comma(), t(' 0 en otro caso')), t('A cero / To zero')),
            (m(n(0), t(' si / if / se '), I, o('&gt;'), T_, comma(), t(' I en otro caso')), t('A cero invertido / To zero inverted')),
        ])), L('En color, cada canal se compara por separado.', 'In colour images each channel is compared separately.', 'A colori, ogni canale si confronta separatamente.')),
    ],
    'params': {
        'level': L('p, de 0 a 100 %.', 'p, from 0 to 100 %.', 'p, da 0 a 100 %.'),
        'type': L('Regla de la tabla anterior.', 'Rule from the table above.', 'Regola della tabella sopra.'),
    },
})

# --------------------------------------------------------------------------- brillo / contraste
PAGES.append({
    'id': 'filter.brightness_contrast',
    'intro': L('Transformación lineal de la intensidad.', 'Linear intensity transform.', "Trasformazione lineare dell'intensità."),
    'blocks': [
        ('formula', MATH(Ip, eq(), iv('sat'), par(alpha, I, o('+'), beta), comma(), sp_('1em'), beta, eq(), fr(i('b'), n(100)), times(), M),
         L('sat() recorta el resultado al intervalo [0, M] (255 o 65535). α &gt; 1 estira el histograma alrededor de 0; si quiere aumentar el contraste sin oscurecer, combine α &gt; 1 con un brillo negativo.',
           'sat() clips the result to [0, M] (255 or 65535). α &gt; 1 stretches the histogram around 0; to add contrast without darkening, combine α &gt; 1 with a negative brightness.',
           "sat() taglia il risultato all'intervallo [0, M] (255 o 65535). α &gt; 1 allunga l'istogramma attorno a 0; per aumentare il contrasto senza scurire, combini α &gt; 1 con una luminosità negativa.")),
    ],
    'params': {
        'brightness': L('b, desplazamiento en % del rango (−100 a 100).', 'b, offset as % of the range (−100 to 100).', "b, spostamento in % dell'intervallo (da −100 a 100)."),
        'contrast': L('α, factor multiplicativo (0 a 4).', 'α, multiplicative factor (0 to 4).', 'α, fattore moltiplicativo (da 0 a 4).'),
    },
})

# --------------------------------------------------------------------------- morfología
ominus, oplus = o('⊖'), o('⊕')
PAGES.append({
    'id': 'filter.morphology',
    'intro': L('Operaciones de morfología matemática con un elemento estructurante K (la forma del núcleo). En imágenes binarias, la erosión adelgaza las formas y la dilatación las engorda.',
               'Mathematical morphology operations with a structuring element K (the kernel shape). On binary images, erosion thins shapes and dilation thickens them.',
               "Operazioni di morfologia matematica con un elemento strutturante K (la forma del nucleo). Nelle immagini binarie l'erosione assottiglia le forme e la dilatazione le ingrossa."),
    'blocks': [
        ('formula', MATH(call(par(I, ominus, K_), i('x')), eq(), under(iv('min'), m(i('s'), o('∈'), K_)), call(I, m(i('x'), o('+'), i('s'))), comma(), sp_('1.5em'),
                         call(par(I, oplus, K_), i('x')), eq(), under(iv('max'), m(i('s'), o('∈'), K_)), call(I, m(i('x'), o('+'), i('s')))),
         L('Erosión (mínimo local) y dilatación (máximo local).', 'Erosion (local minimum) and dilation (local maximum).', 'Erosione (minimo locale) e dilatazione (massimo locale).')),
        ('formula', MATH(i('I'), o('∘'), K_, eq(), par(I, ominus, K_), oplus, K_, comma(), sp_('1em'),
                         I, o('•'), K_, eq(), par(I, oplus, K_), ominus, K_, comma(), sp_('1em'),
                         iv('grad'), par(I), eq(), par(I, oplus, K_), o('−'), par(I, ominus, K_)),
         L('Apertura (quita salientes y manchas menores que K), cierre (tapa huecos y grietas menores que K) y gradiente morfológico (contorno de las formas).',
           'Opening (removes protrusions and specks smaller than K), closing (fills holes and gaps smaller than K) and morphological gradient (outline of the shapes).',
           'Apertura (toglie sporgenze e macchie più piccole di K), chiusura (chiude buchi e fessure più piccoli di K) e gradiente morfologico (contorno delle forme).')),
    ],
    'params': {
        'operation': L('Una de las cinco operaciones anteriores.', 'One of the five operations above.', 'Una delle cinque operazioni sopra.'),
        'shape': L('Forma de K: rectángulo, elipse inscrita o cruz.', 'Shape of K: rectangle, inscribed ellipse or cross.', 'Forma di K: rettangolo, ellisse inscritta o croce.'),
        'ksize': L('Lado de K en píxeles.', 'Side of K in pixels.', 'Lato di K in pixel.'),
        'iterations': L('n: la erosión o dilatación se repite n veces (en la apertura, n erosiones y luego n dilataciones).',
                        'n: erosion or dilation is repeated n times (for opening, n erosions then n dilations).',
                        "n: l'erosione o la dilatazione si ripete n volte (nell'apertura, n erosioni e poi n dilatazioni)."),
    },
})

# --------------------------------------------------------------------------- formato
R_, G_, B_, Y_ = i('R'), i('G'), i('B'), i('Y')
v_ = i('v')
PAGES.append({
    'id': 'format',
    'title': L('Cambiar formato', 'Change format', 'Cambia formato'),
    'intro': L('Las imágenes se guardan con 8 bits (0–255) o 16 bits (0–65535) por canal, en grises (1 canal) o color (3 canales, más alfa opcional).',
               'Images are stored with 8 bits (0–255) or 16 bits (0–65535) per channel, as grayscale (1 channel) or colour (3 channels, plus optional alpha).',
               'Le immagini si memorizzano con 8 bit (0–255) o 16 bit (0–65535) per canale, in grigi (1 canale) o a colori (3 canali, più alfa opzionale).'),
    'blocks': [
        ('formula', MATH(sub(v_, n(16)), eq(), n(257), times(), sub(v_, n(8)), comma(), sp_('1.5em'), sub(v_, n(8)), eq(), iv('round'), par(fr(sub(v_, n(16)), n(257)))),
         L('257 = 65535/255, así que 255 pasa exactamente a 65535 y la conversión de ida y vuelta no pierde nada. De 16 a 8 bits sí se pierde precisión.',
           '257 = 65535/255, so 255 maps exactly to 65535 and a round trip loses nothing. Going from 16 to 8 bits does lose precision.',
           '257 = 65535/255, quindi 255 diventa esattamente 65535 e la conversione andata e ritorno non perde nulla. Da 16 a 8 bit invece si perde precisione.')),
        ('formula', MATH(Y_, eq(), n('0.299'), R_, o('+'), n('0.587'), G_, o('+'), n('0.114'), B_),
         L('Luminancia (ITU-R BT.601) al pasar a escala de grises; el canal alfa se descarta.',
           'Luminance (ITU-R BT.601) when converting to grayscale; the alpha channel is dropped.',
           'Luminanza (ITU-R BT.601) nel passaggio a scala di grigi; il canale alfa si scarta.')),
        ('formula', MATH(R_, eq(), G_, eq(), B_, eq(), Y_),
         L('Al pasar de grises a color, los tres canales toman el mismo valor (el aspecto no cambia, pero permite aplicar operaciones de color).',
           'From grayscale to colour, the three channels take the same value (the look does not change, but colour operations become possible).',
           "Da grigi a colori, i tre canali prendono lo stesso valore (l'aspetto non cambia, ma si possono applicare operazioni a colori).")),
    ],
    'params': {},
})
