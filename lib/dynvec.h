#ifndef _dynvec_h_
#define _dynvec_h_

struct dynvec
{
  void **dv_vec;
  int dv_capacity;
  int dv_fill;
};

struct dynvec *make_dynvec __P((int n));
void dynvec_free __P((struct dynvec *dv));
void dynvec_freeze __P((struct dynvec *dv));
void dynvec_append __P((struct dynvec *dv, void *element));

#endif /* not _dynvec_h_ */
