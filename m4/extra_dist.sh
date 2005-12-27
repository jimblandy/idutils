#! /bin/sh

echo "EXTRA_DIST = \\" > Makefile.am

for NAME in *.m4; do
  echo "        $NAME \\" >> Makefile.am
done

echo "        extra_dist.sh" >> Makefile.am


