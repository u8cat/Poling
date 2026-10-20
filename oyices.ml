(* Poling: Proof Of Linearizability Generator
 * See LICENSE.txt for license.
 *)

(* Yices 1 style interface on top of Yices 2; see oyices.mli.
   Yices 2 types and terms are global 32-bit identifiers, represented here as
   OCaml ints; contexts are custom blocks owned by the C side. *)

exception Yices_error of string

let () = Callback.register_exception "Oyices.Yices_error" (Yices_error "")

external init : unit -> unit = "oyices_init"
let () = init ()

type yices_context
type yices_type = int
type yices_var_decl = int
type yices_expr = int

let log_warned = ref false

(* Yices 2 has no equivalent of Yices 1's API log file. *)
let yices_enable_log_file (_ : string) =
  if not !log_warned then begin
    log_warned := true;
    prerr_endline "Warning: query logging is not supported with Yices 2; ignoring."
  end

external yices_mk_context : unit -> yices_context = "oyices_mk_context"
external yices_reset : yices_context -> unit = "oyices_reset"
external yices_push : yices_context -> unit = "oyices_push"
external yices_pop : yices_context -> unit = "oyices_pop"
external yices_assert : yices_context -> yices_expr -> unit = "oyices_assert"
external yices_inconsistent : yices_context -> int = "oyices_inconsistent"
external yices_check : yices_context -> int = "oyices_check"

external mk_type : string -> yices_type = "oyices_mk_type"
external function_type : yices_type array -> yices_type -> yices_type
  = "oyices_function_type"
external new_uninterpreted_term : yices_type -> yices_var_decl
  = "oyices_new_uninterpreted_term"

external yices_true : unit -> yices_expr = "oyices_true"
external yices_false : unit -> yices_expr = "oyices_false"
external yices_not : yices_expr -> yices_expr = "oyices_not"
external yices_and : yices_expr array -> yices_expr = "oyices_and"
external yices_or : yices_expr array -> yices_expr = "oyices_or"
external yices_eq : yices_expr -> yices_expr -> yices_expr = "oyices_eq"
external yices_neq : yices_expr -> yices_expr -> yices_expr = "oyices_neq"
external yices_application : yices_expr -> yices_expr array -> yices_expr
  = "oyices_application"
external yices_int : int -> yices_expr = "oyices_int"
external yices_sum : yices_expr array -> yices_expr = "oyices_sum"
external yices_sub : yices_expr -> yices_expr -> yices_expr = "oyices_sub"
external yices_product : yices_expr array -> yices_expr = "oyices_product"
external yices_lt : yices_expr -> yices_expr -> yices_expr = "oyices_lt"
external yices_le : yices_expr -> yices_expr -> yices_expr = "oyices_le"
external yices_gt : yices_expr -> yices_expr -> yices_expr = "oyices_gt"
external yices_ge : yices_expr -> yices_expr -> yices_expr = "oyices_ge"

let yices_mk_type _ name = mk_type name
let yices_mk_function_type _ dom range = function_type dom range

(* Yices 1 declarations are named; the names are only used for printing,
   so a fresh uninterpreted term is all we need. *)
let yices_mk_var_decl _ (_ : string) ty = new_uninterpreted_term ty
let yices_mk_var_from_decl _ decl = decl

let yices_mk_true _ = yices_true ()
let yices_mk_false _ = yices_false ()
let yices_mk_not _ e = yices_not e
let yices_mk_and _ es = yices_and es
let yices_mk_or _ es = yices_or es
let yices_mk_eq _ e1 e2 = yices_eq e1 e2
let yices_mk_diseq _ e1 e2 = yices_neq e1 e2

let yices_mk_app _ f args = yices_application f args
let yices_mk_num _ n = yices_int n
let yices_mk_sum _ es = yices_sum es

(* Yices 1: args.(0) - args.(1) - ... - args.(n-1) *)
let yices_mk_sub _ es =
  let n = Array.length es in
  if n = 0 then raise (Yices_error "yices_mk_sub: no arguments")
  else Array.fold_left yices_sub es.(0) (Array.sub es 1 (n - 1))

let yices_mk_mul _ es = yices_product es
let yices_mk_lt _ e1 e2 = yices_lt e1 e2
let yices_mk_le _ e1 e2 = yices_le e1 e2
let yices_mk_gt _ e1 e2 = yices_gt e1 e2
let yices_mk_ge _ e1 e2 = yices_ge e1 e2
