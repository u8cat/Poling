(* Poling: Proof Of Linearizability Generator
 * See LICENSE.txt for license.
 *)

(** Minimal OCaml binding exposing the subset of the Yices 1 C API used by
    [TheoremProverYices], implemented on top of the Yices 2 library.

    The original Poling distribution linked against a SWIG binding of
    Yices 1.0.x ([oyices.cma]), which SRI no longer distributes.  This
    module keeps the Yices 1 names and calling conventions, so
    [theoremProverYices.ml] can use it unchanged.  Errors reported by
    Yices 2 are raised as [Yices_error]. *)

exception Yices_error of string

type yices_context
type yices_type = private int  (* Yices 2 type identifier *)
type yices_var_decl
type yices_expr

val yices_enable_log_file : string -> unit

val yices_mk_context : unit -> yices_context
val yices_reset : yices_context -> unit
val yices_push : yices_context -> unit
val yices_pop : yices_context -> unit
val yices_assert : yices_context -> yices_expr -> unit

(** 1 if the context is known to be inconsistent, 0 otherwise. *)
val yices_inconsistent : yices_context -> int

(** Yices 1 [lbool]: -1 = unsat, 0 = unknown, 1 = sat. *)
val yices_check : yices_context -> int

(** ["int"], ["real"] and ["bool"] denote the built-in types; any other
    name denotes an uninterpreted type, created on first use. *)
val yices_mk_type : yices_context -> string -> yices_type
val yices_mk_function_type :
  yices_context -> yices_type array -> yices_type -> yices_type

val yices_mk_var_decl : yices_context -> string -> yices_type -> yices_var_decl
val yices_mk_var_from_decl : yices_context -> yices_var_decl -> yices_expr

val yices_mk_true : yices_context -> yices_expr
val yices_mk_false : yices_context -> yices_expr
val yices_mk_not : yices_context -> yices_expr -> yices_expr
val yices_mk_and : yices_context -> yices_expr array -> yices_expr
val yices_mk_or : yices_context -> yices_expr array -> yices_expr
val yices_mk_eq : yices_context -> yices_expr -> yices_expr -> yices_expr
val yices_mk_diseq : yices_context -> yices_expr -> yices_expr -> yices_expr

val yices_mk_app : yices_context -> yices_expr -> yices_expr array -> yices_expr
val yices_mk_num : yices_context -> int -> yices_expr
val yices_mk_sum : yices_context -> yices_expr array -> yices_expr
val yices_mk_sub : yices_context -> yices_expr array -> yices_expr
val yices_mk_mul : yices_context -> yices_expr array -> yices_expr
val yices_mk_lt : yices_context -> yices_expr -> yices_expr -> yices_expr
val yices_mk_le : yices_context -> yices_expr -> yices_expr -> yices_expr
val yices_mk_gt : yices_context -> yices_expr -> yices_expr -> yices_expr
val yices_mk_ge : yices_context -> yices_expr -> yices_expr -> yices_expr
