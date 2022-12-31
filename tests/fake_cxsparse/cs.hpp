// The full CXSparse comes with SuiteSparse which is a giant package.
// Avoid fetching all that by badly hacking just enough pieces to test the loader.
//
// CXSparse is written in C. They support different index and value types by duplicating structures and methods
// and using a suffix on the structure name. The `dl` in `cs_dl` means "double, long" so double value type
// and int64_t index type. 'c' means complex<double>, 'i' means int32_t. To avoid duplicating source code, CXSparse
// uses macros to expand each implementation into each supported type combination.
//
// Here we just support one CXSparse matrix type.
// The index and value types are automatically discovered using the C++ type system, so the other types will work
// as well.
//
// SuiteSparse: https://people.engr.tamu.edu/davis/suitesparse.html
// CXSparse: https://github.com/DrTimothyAldenDavis/SuiteSparse/tree/stable/CXSparse
//
// CXSparse, Copyright (c) 2006-2022, Timothy A. Davis.
// CXSparse is released under a LGPL-2.1+ license.

#include <cstdint>
#include <cstdlib>

#define CS_MAX(a,b) (((a) > (b)) ? (a) : (b))

typedef int64_t csi;

struct cs_dl  /* matrix in compressed-column or triplet form */
{
    int64_t nzmax ; /* maximum number of entries */
    int64_t m ;     /* number of rows */
    int64_t n ;     /* number of columns */
    int64_t *p ;  /* column pointers (size n+1) or col indlces (size nzmax) */
    int64_t *i ;  /* row indices, size nzmax */
    double *x ;     /* numerical values, size nzmax */
    int64_t nz ;  /* # of entries in triplet matrix, -1 for compressed-col */
};


inline cs_dl * cs_dl_spfree (cs_dl *A) {
    if (!A) return nullptr;     /* do nothing if A already NULL */
    free (A->p) ;
    free (A->i) ;
    free (A->x) ;
    free (A);
    return nullptr;
}

inline cs_dl *cs_dl_spalloc (int64_t m, int64_t n, int64_t nzmax,
                      int64_t values, int64_t triplet) {
    cs_dl *A = (cs_dl*)calloc (1, sizeof (cs_dl)) ;    /* allocate the cs struct */
    if (!A) return (nullptr) ;                 /* out of memory */
    A->m = m ;                              /* define dimensions and nzmax */
    A->n = n ;
    A->nzmax = nzmax = CS_MAX (nzmax, 1) ;
    A->nz = triplet ? 0 : -1 ;              /* allocate triplet or comp.col */
    A->p = (int64_t*)malloc ((triplet ? nzmax : n+1) * sizeof (int64_t)) ;
    A->i = (int64_t*)malloc (nzmax * sizeof (int64_t)) ;
    A->x = values ? (double*)malloc (nzmax * sizeof (double)) : nullptr ;
    return ((!A->p || !A->i || (values && !A->x)) ? cs_dl_spfree (A) : A) ;
}

inline int64_t cs_dl_entry (cs_dl *T, int64_t i, int64_t j, double x) {
    // skip re-allocation tests

    if (T->x) T->x [T->nz] = x ;
    T->i [T->nz] = i ;
    T->p [T->nz++] = j ;
    T->m = CS_MAX (T->m, i+1) ;
    T->n = CS_MAX (T->n, j+1) ;
    return (1) ;
}

#define CS_TRIPLET(A) (A && (A->nz >= 0))

csi cs_cumsum (csi *p, csi *c, csi n)
{
    csi i, nz = 0 ;
    if (!p || !c) return (-1) ;     /* check inputs */
    for (i = 0 ; i < n ; i++)
    {
        p [i] = nz ;
        nz += c [i] ;
        c [i] = p [i] ;             /* also copy p[0..n-1] back into c[0..n-1]*/
    }
    p [n] = nz ;
    return (nz) ;                  /* return sum (c [0..n-1]) */
}

cs_dl *cs_dl_compress (const cs_dl *T)
{
    csi m, n, nz, p, k, *Cp, *Ci, *w, *Ti, *Tj ;
    double *Cx, *Tx ;
    cs_dl *C ;
    if (!CS_TRIPLET (T)) return nullptr ;                /* check inputs */

    m = T->m ; n = T->n ; Ti = T->i ; Tj = T->p ; Tx = T->x ; nz = T->nz ;
    C = cs_dl_spalloc (m, n, nz, Tx != nullptr, 0) ;          /* allocate result */
    w = (csi*)calloc (n, sizeof (csi)) ;                   /* get workspace */
    if (!C || !w) return nullptr ;    /* out of memory */
    Cp = C->p ; Ci = C->i ; Cx = C->x ;
    for (k = 0 ; k < nz ; k++) w [Tj [k]]++ ;           /* column counts */
    cs_cumsum (Cp, w, n) ;                              /* column pointers */
    for (k = 0 ; k < nz ; k++)
    {
    Ci [p = w [Tj [k]]++] = Ti [k] ;    /* A(i,j) is the pth entry in C */
    if (Cx) Cx [p] = Tx [k] ;
    }

    free(w);
    return C;      /* success; free w and return C */
}