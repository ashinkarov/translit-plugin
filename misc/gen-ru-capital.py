# -*- coding: utf-8 -*-

s = unicode ('а', 'utf-8')
c = unicode ('А', 'utf-8')

print "INPUT (\"ё\", \"Ё\")"

for i in xrange (32):
        print "INPUT (\"%s\", \"%s\")" % (s.encode ('utf-8'), c.encode ('utf-8'));
        s = unichr (ord (s) + 1)
        c = unichr (ord (c) + 1)

