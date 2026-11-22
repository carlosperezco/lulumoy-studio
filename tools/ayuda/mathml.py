"""Pequeñas funciones para escribir MathML de forma compacta."""


def m(*xs):
    return '<mrow>' + ''.join(xs) + '</mrow>'


def i(x):
    return f'<mi>{x}</mi>'


def iv(x):  # identificador en redonda (funciones: med, sat, atan2...)
    return f'<mi mathvariant="normal">{x}</mi>'


def n(x):
    return f'<mn>{x}</mn>'


def o(x):
    return f'<mo>{x}</mo>'


def t(x):
    return f'<mtext>{x}</mtext>'


def sp_(x):  # espacio
    return f'<mspace width="{x}"/>'


def fr(a, b):
    return f'<mfrac>{a}{b}</mfrac>'


def sup(a, b):
    return f'<msup>{a}{b}</msup>'


def sub(a, b):
    return f'<msub>{a}{b}</msub>'


def subsup(a, b, c):
    return f'<msubsup>{a}{b}{c}</msubsup>'


def under(a, b):
    return f'<munder>{a}{b}</munder>'


def underover(a, b, c):
    return f'<munderover>{a}{b}{c}</munderover>'


def sqrt(a):
    return f'<msqrt>{a}</msqrt>'


def par(*x):
    return m(o('('), *x, o(')'))


def brk(*x):
    return m(o('['), *x, o(']'))


def brace(*x):
    return m(o('{'), *x, o('}'))


def absv(*x):
    return m(o('|'), *x, o('|'))


def norm(*x):
    return m(o('‖'), *x, o('‖'))


def floor(*x):
    return m(o('⌊'), *x, o('⌋'))


def ceil(*x):
    return m(o('⌈'), *x, o('⌉'))


def matrix(rows):
    body = ''.join('<mtr>' + ''.join(f'<mtd>{c}</mtd>' for c in r) + '</mtr>' for r in rows)
    return m(o('['), f'<mtable>{body}</mtable>', o(']'))


def cases(rows):
    """rows: lista de (expresión, condición)."""
    body = ''.join(f'<mtr><mtd>{e}</mtd><mtd>{sp_("1em")}{c}</mtd></mtr>' for e, c in rows)
    return m(o('{'), f'<mtable columnalign="left left">{body}</mtable>')


def comma():
    return o(',')


def eq():
    return o('=')


def times():
    return o('·')


def prime(x):
    return sup(x, o('′'))


def call(f, *args):
    """f(a, b, ...) con f ya en MathML."""
    parts = []
    for k, a in enumerate(args):
        if k:
            parts.append(comma())
        parts.append(a)
    return m(f, par(*parts))


def MATH(*x):
    return '<math display="block">' + m(*x) + '</math>'


def inline(*x):
    return '<math>' + m(*x) + '</math>'


# Símbolos frecuentes
I, x, y, k, r, s = i('I'), i('x'), i('y'), i('k'), i('r'), i('s')
Ip = prime(I)
sigma = i('σ')
M = i('M')


def Ixy(a=None, b=None):
    return call(I, a or x, b or y)


def Ipxy(a=None, b=None):
    return call(Ip, a or x, b or y)


def sum2(lo, hi, var1='i', var2='j'):
    return m(underover(o('∑'), m(i(var1), eq(), lo), hi), underover(o('∑'), m(i(var2), eq(), lo), hi))
