/* Poling: Proof Of Linearizability Generator
 * See LICENSE.txt for license.
 *
 * C stubs for oyices.ml: the subset of Yices 2 needed to emulate the
 * Yices 1 API that Poling was originally written against.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <caml/alloc.h>
#include <caml/callback.h>
#include <caml/custom.h>
#include <caml/fail.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>

#include <yices.h>

/* Yices 2.7 renamed the smt_status_t constants from STATUS_* to
   YICES_STATUS_*. */
#if __YICES_VERSION_MAJOR < 7
#define YICES_STATUS_SAT   STATUS_SAT
#define YICES_STATUS_UNSAT STATUS_UNSAT
#define YICES_STATUS_ERROR STATUS_ERROR
#endif

/* ------------------------------------------------------------------ */
/* Errors                                                              */

static void oyices_fail(const char *what)
{
  char msg[512];
  char *err = yices_error_string();
  snprintf(msg, sizeof msg, "%s: %s", what, err != NULL ? err : "unknown error");
  if (err != NULL) yices_free_string(err);
  yices_clear_error();
  const value *exn = caml_named_value("Oyices.Yices_error");
  if (exn == NULL) caml_failwith(msg);
  caml_raise_with_string(*exn, msg);
}

static value check_term(term_t t, const char *what)
{
  if (t == NULL_TERM) oyices_fail(what);
  return Val_int(t);
}

static value check_type(type_t t, const char *what)
{
  if (t == NULL_TYPE) oyices_fail(what);
  return Val_int(t);
}

/* Copy an OCaml int array of terms/types into a C buffer.  Arguments
   to the Yices 1 API calls used by Poling are small, so the stack buffer
   almost always suffices. */
#define SMALL_ARRAY 16

static int32_t *ids_of_array(value arr, int32_t *small, uint32_t *n)
{
  uint32_t i, len = Wosize_val(arr);
  int32_t *ids = small;
  if (len > SMALL_ARRAY) {
    ids = malloc(len * sizeof(int32_t));
    if (ids == NULL) caml_raise_out_of_memory();
  }
  for (i = 0; i < len; i++) ids[i] = Int_val(Field(arr, i));
  *n = len;
  return ids;
}

static void free_ids(int32_t *ids, int32_t *small)
{
  if (ids != small) free(ids);
}

/* ------------------------------------------------------------------ */
/* Library and contexts                                                */

CAMLprim value oyices_init(value unit)
{
  static int initialized = 0;
  if (!initialized) {
    yices_init();
    initialized = 1;
  }
  return Val_unit;
}

#define Context_val(v) (*((context_t **) Data_custom_val(v)))

static void oyices_finalize_context(value v)
{
  context_t *ctx = Context_val(v);
  if (ctx != NULL) yices_free_context(ctx);
}

static struct custom_operations oyices_context_ops = {
  "poling.oyices.context",
  oyices_finalize_context,
  custom_compare_default,
  custom_hash_default,
  custom_serialize_default,
  custom_deserialize_default,
  custom_compare_ext_default,
  custom_fixed_length_default
};

CAMLprim value oyices_mk_context(value unit)
{
  CAMLparam1(unit);
  CAMLlocal1(v);
  /* Default configuration: push/pop, uninterpreted functions and
     linear integer/real arithmetic. */
  context_t *ctx = yices_new_context(NULL);
  if (ctx == NULL) oyices_fail("yices_new_context");
  v = caml_alloc_custom(&oyices_context_ops, sizeof(context_t *), 0, 1);
  Context_val(v) = ctx;
  CAMLreturn(v);
}

CAMLprim value oyices_reset(value ctx)
{
  yices_reset_context(Context_val(ctx));
  return Val_unit;
}

CAMLprim value oyices_push(value ctx)
{
  if (yices_push(Context_val(ctx)) < 0) oyices_fail("yices_push");
  return Val_unit;
}

CAMLprim value oyices_pop(value ctx)
{
  if (yices_pop(Context_val(ctx)) < 0) oyices_fail("yices_pop");
  return Val_unit;
}

CAMLprim value oyices_assert(value ctx, value t)
{
  if (yices_assert_formula(Context_val(ctx), Int_val(t)) < 0)
    oyices_fail("yices_assert_formula");
  return Val_unit;
}

CAMLprim value oyices_inconsistent(value ctx)
{
  return Val_int(yices_context_status(Context_val(ctx)) == YICES_STATUS_UNSAT);
}

/* Returns a Yices 1 lbool: -1 (unsat), 0 (unknown), 1 (sat). */
CAMLprim value oyices_check(value ctx)
{
  switch (yices_check_context(Context_val(ctx), NULL)) {
  case YICES_STATUS_SAT:
    return Val_int(1);
  case YICES_STATUS_UNSAT:
    return Val_int(-1);
  case YICES_STATUS_ERROR:
    oyices_fail("yices_check_context");
  default:
    return Val_int(0);
  }
}

/* ------------------------------------------------------------------ */
/* Types                                                               */

CAMLprim value oyices_mk_type(value name)
{
  const char *s = String_val(name);
  type_t tau;
  if (strcmp(s, "int") == 0) return check_type(yices_int_type(), s);
  if (strcmp(s, "real") == 0) return check_type(yices_real_type(), s);
  if (strcmp(s, "bool") == 0) return check_type(yices_bool_type(), s);
  /* Yices 1 creates an uninterpreted type for an unknown name. */
  tau = yices_get_type_by_name(s);
  if (tau == NULL_TYPE) {
    tau = yices_new_uninterpreted_type();
    if (tau == NULL_TYPE || yices_set_type_name(tau, s) < 0)
      oyices_fail("yices_mk_type");
  }
  return Val_int(tau);
}

CAMLprim value oyices_function_type(value dom, value range)
{
  int32_t small[SMALL_ARRAY];
  uint32_t n;
  int32_t *ids = ids_of_array(dom, small, &n);
  type_t tau = yices_function_type(n, ids, Int_val(range));
  free_ids(ids, small);
  return check_type(tau, "yices_function_type");
}

/* ------------------------------------------------------------------ */
/* Terms                                                               */

CAMLprim value oyices_new_uninterpreted_term(value tau)
{
  return check_term(yices_new_uninterpreted_term(Int_val(tau)),
                    "yices_new_uninterpreted_term");
}

CAMLprim value oyices_true(value unit)
{
  return check_term(yices_true(), "yices_true");
}

CAMLprim value oyices_false(value unit)
{
  return check_term(yices_false(), "yices_false");
}

CAMLprim value oyices_not(value t)
{
  return check_term(yices_not(Int_val(t)), "yices_not");
}

CAMLprim value oyices_and(value args)
{
  int32_t small[SMALL_ARRAY];
  uint32_t n;
  int32_t *ids = ids_of_array(args, small, &n);
  term_t t = yices_and(n, ids);
  free_ids(ids, small);
  return check_term(t, "yices_and");
}

CAMLprim value oyices_or(value args)
{
  int32_t small[SMALL_ARRAY];
  uint32_t n;
  int32_t *ids = ids_of_array(args, small, &n);
  term_t t = yices_or(n, ids);
  free_ids(ids, small);
  return check_term(t, "yices_or");
}

CAMLprim value oyices_eq(value t1, value t2)
{
  return check_term(yices_eq(Int_val(t1), Int_val(t2)), "yices_eq");
}

CAMLprim value oyices_neq(value t1, value t2)
{
  return check_term(yices_neq(Int_val(t1), Int_val(t2)), "yices_neq");
}

CAMLprim value oyices_application(value f, value args)
{
  int32_t small[SMALL_ARRAY];
  uint32_t n;
  int32_t *ids = ids_of_array(args, small, &n);
  term_t t = yices_application(Int_val(f), n, ids);
  free_ids(ids, small);
  return check_term(t, "yices_application");
}

CAMLprim value oyices_int(value n)
{
  return check_term(yices_int64(Long_val(n)), "yices_int64");
}

CAMLprim value oyices_sum(value args)
{
  int32_t small[SMALL_ARRAY];
  uint32_t n;
  int32_t *ids = ids_of_array(args, small, &n);
  term_t t = yices_sum(n, ids);
  free_ids(ids, small);
  return check_term(t, "yices_sum");
}

CAMLprim value oyices_sub(value t1, value t2)
{
  return check_term(yices_sub(Int_val(t1), Int_val(t2)), "yices_sub");
}

CAMLprim value oyices_product(value args)
{
  int32_t small[SMALL_ARRAY];
  uint32_t n;
  int32_t *ids = ids_of_array(args, small, &n);
  term_t t = yices_product(n, ids);
  free_ids(ids, small);
  return check_term(t, "yices_product");
}

CAMLprim value oyices_lt(value t1, value t2)
{
  return check_term(yices_arith_lt_atom(Int_val(t1), Int_val(t2)), "yices_arith_lt_atom");
}

CAMLprim value oyices_le(value t1, value t2)
{
  return check_term(yices_arith_leq_atom(Int_val(t1), Int_val(t2)), "yices_arith_leq_atom");
}

CAMLprim value oyices_gt(value t1, value t2)
{
  return check_term(yices_arith_gt_atom(Int_val(t1), Int_val(t2)), "yices_arith_gt_atom");
}

CAMLprim value oyices_ge(value t1, value t2)
{
  return check_term(yices_arith_geq_atom(Int_val(t1), Int_val(t2)), "yices_arith_geq_atom");
}
