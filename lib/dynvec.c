#include <config.h>
#include "dynvec.h"
#include "alloc.h"

struct dynvec *
make_dynvec (int n)
{
  struct dynvec *dv = MALLOC (struct dynvec, 1);
  dv->dv_vec = MALLOC (void *, n);
  dv->dv_capacity = n;
  dv->dv_fill = 0;
  return dv;
}

void
dynvec_free (struct dynvec *dv)
{
  free (dv->dv_vec);
  free (dv);
}

void
dynvec_freeze (struct dynvec *dv)
{
  if (dv->dv_fill == dv->dv_capacity)
    return;
  dv->dv_capacity = dv->dv_fill;
  dv->dv_vec = REALLOC (dv->dv_vec, void *, dv->dv_capacity);
}

void
dynvec_append (struct dynvec *dv, void *element)
{
  if (dv->dv_fill == dv->dv_capacity)
    {
      dv->dv_capacity *= 2;
      dv->dv_vec = REALLOC (dv->dv_vec, void *, dv->dv_capacity);
    }
  dv->dv_vec[dv->dv_fill++] = element;
}
