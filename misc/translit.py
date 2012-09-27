#!/usr/bin/python
# -*- coding: utf-8 -*-

trans = {}
def INPUT (eng, ru):
        trans[unicode (ru, 'utf-8')] = unicode (eng, 'utf-8')

INPUT("Shh", "Щ")
INPUT("Zh", "Ж")
INPUT("Yu", "Ю")
INPUT("Yo", "Ё")
INPUT("Ya", "Я")
INPUT("Y'", "Ы")
INPUT("Sh", "Ш")
INPUT("E'", "Э")
INPUT("Cz", "Ц")
INPUT("Ch", "Ч")
INPUT("C", "Ц")
INPUT("Z", "З")
INPUT("X", "Х")
INPUT("V", "В")
INPUT("U", "У")
INPUT("T", "Т")
INPUT("S", "С")
INPUT("R", "Р")
INPUT("P", "П")
INPUT("O", "О")
INPUT("N", "Н")
INPUT("M", "М")
INPUT("L", "Л")
INPUT("K", "К")
INPUT("J", "Й")
INPUT("I", "И")
INPUT("H", "Х")
INPUT("G", "Г")
INPUT("F", "Ф")
INPUT("E", "Е")
INPUT("D", "Д")
INPUT("B", "Б")
INPUT("A", "А")
INPUT("shh", "щ")
INPUT("zh", "ж")
INPUT("yu", "ю")
INPUT("yo", "ё")
INPUT("ya", "я")
INPUT("y'", "ы")
INPUT("sh", "ш")
INPUT("e'", "э")
INPUT("cz", "ц")
INPUT("ch", "ч")
INPUT("''", "ъ")
INPUT("c", "ц")
INPUT("z", "з")
INPUT("x", "х")
INPUT("v", "в")
INPUT("u", "у")
INPUT("t", "т")
INPUT("s", "с")
INPUT("r", "р")
INPUT("p", "п")
INPUT("o", "о")
INPUT("n", "н")
INPUT("m", "м")
INPUT("l", "л")
INPUT("k", "к")
INPUT("j", "й")
INPUT("i", "и")
INPUT("h", "х")
INPUT("g", "г")
INPUT("f", "ф")
INPUT("e", "е")
INPUT("d", "д")
INPUT("b", "б")
INPUT("a", "а")
INPUT("'", "ь")

#for k in trans:
#        print k, trans[k]

import re

words = open ("pw.txt", 'r').read ()
words = unicode (words, 'utf-8')
words = re.split (u'\W+', words, flags=re.UNICODE)

for word in words:
        w = word.strip ()
        #w = unicode (word, 'utf-8').strip ()
        print "%s\t" % w.encode ('utf-8'),
        nw = ""
        for l in w:
                if l in trans:
                        nw += trans[l]
                else:
                        nw += l
        print nw.encode ('utf-8')

