(*
 * lp_solve v5.5 API for Delphi 2009 & FPC compiler v1.9.x
 * Licence LGPL
 *
 * Author: Henri Gourvest
 * email: hgourvest@progdigy.com
 * homepage: http://www.progdigy.com
 * date: 07/21/2004
 *
 * Update for Delphi 2009 by:
 * Thijs Urlings
 * http://soa.iti.es/thijs
 * date: 11/01/2010
 *
 * Important information
 * Solver library is compiled for a different Control Word, you should change
 * the Delphi control Word to avoid foating point operation errors.
 * _control87     Set8087CW
 * Visual Studio $9001F     ->   639
 * GCC           $8001F     ->   895
 *
 *)

{$I lpsolve.inc}


{$ALIGN ON}
{$MINENUMSIZE 4}
{$IFNDEF FPC}
  {$WEAKPACKAGEUNIT}
{$ELSE}
  {$MODE DELPHI}
{$ENDIF}
{$IFDEF LINUX}
  {$DEFINE UNIX}
{$ENDIF}

unit lpsolve;

interface

uses
{$IFDEF LPSOLVE_DYNAMIC}
  {$IFDEF UNIX}
  Libc, dlfcn,
  {$ELSE}
  Windows,
  {$ENDIF}
{$ENDIF}
  SysUtils;

const
  MAXLONG = $7FFFFFFF;

type
  PIntArray = ^TIntArray;
  TIntArray = array[0..MAXLONG div SizeOf(Integer)-1] of Integer;

  PFloatArray = ^TFloatArray;
  TFloatArray = array[0..(MAXLONG div SizeOf(Double))-1] of Double;

  PPtrArray = ^TPtrArray;
  TPtrArray = array[0..(MAXLONG div SizeOf(Pointer))-1] of Pointer;

{$IFNDEF FPC}
{$IFDEF VER130}
  THandle = LongWord;
  Pinteger = ^integer;
  PDouble = ^double;
{$ENDIF}
{$ENDIF}

(* MYBOOL *)
const
  _FALSE      = 0;
  _TRUE       = 1;
  _AUTOMATIC  = 2;
  _DYNAMIC    = 4;

(* Prototypes for call-back functions                                        *)
type
{$IFDEF LPS55_UP}
  lphandle_intfunc = function(lp: THandle; userhandle: Pointer): Integer; stdcall;
  lphandlestr_func = procedure(lp: THandle; userhandle: Pointer; buf: PAnsiChar); stdcall;
  lphandleint_func = procedure(lp: THandle; userhandle: Pointer; message: Integer); stdcall;
  lphandleint_intfunc = function(lp: THandle; userhandle: Pointer; message: Integer): Integer; stdcall;
{$ELSE}
  ctrlcfunc = function(lp: THandle; userhandle: Pointer): integer; stdcall;
  logfunc = procedure(lp: THandle; userhandle: Pointer; buf: PAnsiChar); stdcall;
  msgfunc = procedure(lp: THandle; userhandle: Pointer; message: integer); stdcall;
{$ENDIF}

{$IFDEF LPS55_UP}
  COUNTER = Int64;
{$ELSE}
  COUNTER = Integer;
{$ENDIF}

(* Definition of program constrants                                          *)
const
  SIMPLEX_UNDEFINED       =  0;
  SIMPLEX_Phase1_PRIMAL   =  1;
  SIMPLEX_Phase1_DUAL     =  2;
  SIMPLEX_Phase2_PRIMAL   =  4;
  SIMPLEX_Phase2_DUAL     =  8;
  SIMPLEX_DYNAMIC         = 16;
{$IFDEF LPS55_UP}
  SIMPLEX_AUTODUALIZE     = 32;
{$ENDIF}

  SIMPLEX_PRIMAL_PRIMAL   = SIMPLEX_Phase1_PRIMAL + SIMPLEX_Phase2_PRIMAL;
  SIMPLEX_DUAL_PRIMAL     = SIMPLEX_Phase1_DUAL   + SIMPLEX_Phase2_PRIMAL;
  SIMPLEX_PRIMAL_DUAL     = SIMPLEX_Phase1_PRIMAL + SIMPLEX_Phase2_DUAL;
  SIMPLEX_DUAL_DUAL       = SIMPLEX_Phase1_DUAL   + SIMPLEX_Phase2_DUAL;
  SIMPLEX_DEFAULT         = SIMPLEX_DUAL_PRIMAL;

(* Presolve defines *)
  PRESOLVE_NONE          =      0;
  PRESOLVE_ROWS          =      1;
  PRESOLVE_COLS          =      2;
  PRESOLVE_LINDEP        =      4;
  PRESOLVE_AGGREGATE     =      8;
  PRESOLVE_SPARSER       =     16;
  PRESOLVE_SOS           =     32;
  PRESOLVE_REDUCEMIP     =     64;
{$IFDEF LPS55_UP}
  PRESOLVE_KNAPSACK      =     128;  // Implementation not tested completely
  PRESOLVE_ELIMEQ2       =     256;
  PRESOLVE_IMPLIEDFREE   =     512;
  PRESOLVE_REDUCEGCD     =    1024;
  PRESOLVE_PROBEFIX      =    2048;
  PRESOLVE_PROBEREDUCE   =    4096;
  PRESOLVE_ROWDOMINATE   =    8192;
  PRESOLVE_COLDOMINATE   =   16384;
  PRESOLVE_MERGEROWS     =   32768;
  PRESOLVE_IMPLIEDSLK    =   65536;
  PRESOLVE_COLFIXDUAL    =  131072;
  PRESOLVE_BOUNDS        =  262144;
  PRESOLVE_DUALS         =  524288;
  PRESOLVE_SENSDUALS     = 1048576;
{$ELSE}
  PRESOLVE_DUALS         =     128;
  PRESOLVE_SENSDUALS     =     256;
{$ENDIF}
  PRESOLVE_LASTMASKMODE  = (PRESOLVE_DUALS - 1);

(* Basis crash options *)
  CRASH_NONE              = 0;
  CRASH_NONBASICBOUNDS    = 1;
  CRASH_MOSTFEASIBLE      = 2;
  CRASH_LEASTDEGENERATE   = 3;

(* Strategy codes to avoid or recover from degenerate pivots,
   infeasibility or numeric errors via randomized bound relaxation *)
  ANTIDEGEN_NONE          =   0;
  ANTIDEGEN_FIXEDVARS     =   1;
  ANTIDEGEN_COLUMNCHECK   =   2;
  ANTIDEGEN_STALLING      =   4;
  ANTIDEGEN_NUMFAILURE    =   8;
  ANTIDEGEN_LOSTFEAS      =  16;
  ANTIDEGEN_INFEASIBLE    =  32;
  ANTIDEGEN_DYNAMIC       =  64;
  ANTIDEGEN_DURINGBB      = 128;
{$IFDEF LPS55_UP}
  ANTIDEGEN_RHSPERTURB    = 256;
  ANTIDEGEN_BOUNDFLIP     = 512;
{$ENDIF}
  ANTIDEGEN_DEFAULT       = (ANTIDEGEN_FIXEDVARS or ANTIDEGEN_STALLING or ANTIDEGEN_INFEASIBLE);

(* Constraint type codes *)
  FR                      = 0;
  LE                      = 1;
  GE                      = 2;
  EQ                      = 3;
  OF_                     = 4;

(* Improvement defines *)
{$IFDEF LPS51}
  IMPROVE_NONE            = 0;
  IMPROVE_FTRAN           = 1;
  IMPROVE_BTRAN           = 2;
  IMPROVE_SOLVE           = (IMPROVE_FTRAN + IMPROVE_BTRAN);
  IMPROVE_INVERSE         = 4;
{$ENDIF}
{$IFDEF LPS55}
  IMPROVE_NONE            = 0;
  IMPROVE_SOLUTION        = 1;
  IMPROVE_DUALFEAS        = 2;
  IMPROVE_THETAGAP        = 4;
  IMPROVE_BBSIMPLEX       = 8;
  IMPROVE_DEFAULT         = (IMPROVE_DUALFEAS + IMPROVE_THETAGAP);
  IMPROVE_INVERSE         = (IMPROVE_SOLUTION + IMPROVE_THETAGAP);
{$ENDIF}

(* Scaling types *)
  SCALE_NONE              = 0;
  SCALE_EXTREME           = 1;
  SCALE_RANGE             = 2;
  SCALE_MEAN              = 3;
  SCALE_GEOMETRIC         = 4;
  SCALE_FUTURE1           = 5;
  SCALE_FUTURE2           = 6;
  SCALE_CURTISREID        = 7;   // Override to Curtis-Reid "optimal" scaling

(* Alternative scaling weights *)
  SCALE_LINEAR           =  0;
  SCALE_QUADRATIC        =  8;
  SCALE_LOGARITHMIC      = 16;
  SCALE_USERWEIGHT       = 31;
  SCALE_MAXTYPE          = (SCALE_QUADRATIC-1);

(* Scaling modes *)
  SCALE_POWER2          =  32;   (* As is or rounded to power of 2 *)
  SCALE_EQUILIBRATE     =  64;   (* Make sure that no scaled number is above 1 *)
  SCALE_INTEGERS        = 128;   (* Apply to integer columns/variables *)
  SCALE_DYNUPDATE       = 256;   (* Apply incrementally every solve() *)
{$IFDEF LPS55_UP}
  SCALE_ROWSONLY        = 512;   (* Override any scaling to only scale the rows *)
  SCALE_COLSONLY       = 1024;   (* Override any scaling to only scale the rows *)
{$ENDIF}

(* Pricing methods *)
  PRICER_FIRSTINDEX       = 0;
  PRICER_DANTZIG          = 1;
  PRICER_DEVEX            = 2;
  PRICER_STEEPESTEDGE     = 3;
  PRICER_LASTOPTION       = PRICER_STEEPESTEDGE;

(* Pricing strategies *)
  PRICE_METHODDEFAULT   =    0;
  PRICE_PRIMALFALLBACK  =    4;    (* In case of Steepest Edge, fall back to DEVEX in primal *)
  PRICE_MULTIPLE        =    8;    (* Enable multiple pricing (primal simplex) *)
  PRICE_PARTIAL         =   16;    (* Enable partial pricing (primal simplex) *)
  PRICE_ADAPTIVE        =   32;    (* Temporarily use First Index if cycling is detected *)
  PRICE_HYBRID          =   64;    (* NOT IMPLEMENTED *)
  PRICE_RANDOMIZE       =  128;    (* Adds a small randomization effect to the selected pricer *)
{$IFDEF LPS55_UP}
  PRICE_AUTOPARTIAL     = 256;    (* Detect and use data on the block structure of the model (primal) *)
  PRICE_AUTOMULTIPLE    = 512;    (* Automatically select multiple pricing (primal simplex) *)
{$ELSE}
  PRICE_AUTOPARTIALCOLS =  256;    (* Detect and use data on the block structure of the model (primal) *)
  PRICE_AUTOPARTIALROWS =  512;    (* Detect and use data on the block structure of the model (dual) *)
{$ENDIF}
  PRICE_LOOPLEFT        = 1024;    (* Scan entering/leaving columns left rather than right *)
  PRICE_LOOPALTERNATE   = 2048;    (* Scan entering/leaving columns alternatingly left/right *)
{$IFDEF LPS55_UP}
  PRICE_HARRISTWOPASS   =  4096;    (* Use Harris' primal pivot logic rather than the default *)
  PRICE_FORCEFULL       =  8192;    (* Non-user option to force full pricing *)
  PRICE_TRUENORMINIT    = 16384;    (* Use true norms for Devex and Steepest Edge initializations *)
{$ELSE}
  PRICE_AUTOMULTICOLS   = 4096;    (* Automatically select multiple pricing (primal) *)
  PRICE_AUTOMULTIROWS   = 8192;    (* Automatically select multiple pricing (dual) *)
{$ENDIF}



{$IFDEF LPS55_UP}
  PRICE_STRATEGYMASK    = (PRICE_METHODDEFAULT + PRICE_PRIMALFALLBACK + PRICE_MULTIPLE + PRICE_PARTIAL +
                            PRICE_ADAPTIVE + PRICE_HYBRID + PRICE_RANDOMIZE + PRICE_AUTOPARTIAL +
                            PRICE_AUTOMULTIPLE +  PRICE_HARRISTWOPASS + PRICE_LOOPLEFT + PRICE_LOOPALTERNATE +
                            PRICE_FORCEFULL + PRICE_TRUENORMINIT);
{$ELSE}
  PRICE_AUTOPARTIAL     = (PRICE_AUTOPARTIALCOLS + PRICE_AUTOPARTIALROWS);
  PRICE_AUTOMULTIPLE    = (PRICE_AUTOMULTICOLS + PRICE_AUTOMULTIROWS);
  PRICE_STRATEGYMASK    = (PRICE_METHODDEFAULT + PRICE_PRIMALFALLBACK + PRICE_MULTIPLE + PRICE_PARTIAL +
                            PRICE_ADAPTIVE + PRICE_HYBRID + PRICE_RANDOMIZE + PRICE_AUTOPARTIAL +
                            PRICE_AUTOMULTIPLE + PRICE_LOOPLEFT + PRICE_LOOPALTERNATE);

{$ENDIF}


  PRICER_RANDFACT       = 0.1;

(* B&B strategies *)
  NODE_FIRSTSELECT         =    0;
  NODE_GAPSELECT           =    1;
  NODE_RANGESELECT         =    2;
  NODE_FRACTIONSELECT      =    3;
  NODE_PSEUDOCOSTSELECT    =    4;
  NODE_PSEUDONONINTSELECT  =    5;    (* Kjell Eikland #1 - Minimize B&B depth *)
  NODE_PSEUDORATIOSELECT   =    6;    (* Kjell Eikland #2 - Minimize a "cost/benefit" ratio *)
  NODE_USERSELECT          =    7;
  NODE_WEIGHTREVERSEMODE   =    8;
  NODE_STRATEGYMASK        = (NODE_WEIGHTREVERSEMODE-1); (* Mask for B&B strategies *)
{$IFDEF LPS55_UP}
  NODE_PSEUDOFEASSELECT    = (NODE_PSEUDONONINTSELECT + NODE_WEIGHTREVERSEMODE);
{$ENDIF}
  NODE_BRANCHREVERSEMODE   =   16;
  NODE_GREEDYMODE          =   32;
  NODE_PSEUDOCOSTMODE      =   64;
  NODE_DEPTHFIRSTMODE      =  128;
  NODE_RANDOMIZEMODE       =  256;
  NODE_GUBMODE             =  512;
  NODE_DYNAMICMODE         = 1024;
  NODE_RESTARTMODE         = 2048;
  NODE_BREADTHFIRSTMODE    = 4096;
  NODE_AUTOORDER           = 8192;
{$IFDEF LPS55_UP}
  NODE_RCOSTFIXING         = 16384;
  NODE_STRONGINIT          = 32768;
{$ELSE}
  NODE_PSEUDOFEASSELECT    = (NODE_PSEUDONONINTSELECT+NODE_WEIGHTREVERSEMODE);
{$ENDIF}

  BRANCH_CEILING          = 0;
  BRANCH_FLOOR            = 1;
  BRANCH_AUTOMATIC        = 2;
  BRANCH_DEFAULT          = 3;

(* Solver status values *)
  UNKNOWNERROR            = -5;
  DATAIGNORED             = -4;
  NOBFP                   = -3;
  NOMEMORY                = -2;
  NOTRUN                  = -1;
  OPTIMAL                 =  0;
  SUBOPTIMAL              =  1;
  INFEASIBLE              =  2;
  UNBOUNDED               =  3;
  DEGENERATE              =  4;
  NUMFAILURE              =  5;
  USERABORT               =  6;
  TIMEOUT                 =  7;
  RUNNING                 =  8;
  PRESOLVED               =  9;
  ACCURACYERROR           = 25;

(* Branch & Bound and Lagrangean extra status values *)
  PROCFAIL               = 10;
  PROCBREAK              = 11;
  FEASFOUND              = 12;
  NOFEASFOUND            = 13;
{$IFDEF LPS55_UP}
  FATHOMED               = 14;
{$ENDIF}

(* REPORT defines *)
  NEUTRAL                 = 0;
  CRITICAL                = 1;
  SEVERE                  = 2;
  IMPORTANT               = 3;
  NORMAL                  = 4;
  DETAILED                = 5;
  FULL                    = 6;

(* MESSAGE defines *)
  MSG_NONE             =    0;
  MSG_PRESOLVE         =    1;
  MSG_ITERATION        =    2;
  MSG_INVERT           =    4;
  MSG_LPFEASIBLE       =    8;
  MSG_LPOPTIMAL        =   16;
  MSG_LPEQUAL          =   32;
  MSG_LPBETTER         =   64;
  MSG_MILPFEASIBLE     =  128;
  MSG_MILPEQUAL        =  256;
  MSG_MILPBETTER       =  512;
  MSG_MILPSTRATEGY     = 1024;
  MSG_MILPOPTIMAL      = 2048;
  MSG_PERFORMANCE      = 4096;
  MSG_INITPSEUDOCOST   = 8192;


// Parameters constants for short-cut setting of tolerances
// --------------------------------------------------------------------------------------
  EPS_TIGHT                = 0;
  EPS_MEDIUM               = 1;
  EPS_LOOSE                = 2;
  EPS_BAGGY                = 3;
  EPS_DEFAULT              = EPS_TIGHT;

type

(* Routines with UNIQUE implementations for each XLI engine  *)

  xli_name = function: PAnsiChar; stdcall;
  xli_readmodel = function(lp: THandle; modelname, dataname, options: PAnsiChar; verbose: integer): boolean; stdcall;
  xli_writemodel = function(lp: THandle; filename, options: PAnsiChar; results: boolean): boolean; stdcall;

(* Routines SHARED for all XLI implementations; *)

  xli_compatible = function(lp: Thandle; xliversion, lpversion: Integer): boolean; stdcall;


(* User and system function interfaces                                       *)

procedure lp_solve_version(majorversion: Pinteger; minorversion: Pinteger;
  release: Pinteger; build: Pinteger); stdcall;

function make_lp(rows: integer; columns: integer): THandle; stdcall;
function resize_lp(lp: THandle; rows: integer; columns: integer): boolean; stdcall;
function get_status(lp: THandle): integer; stdcall;
function get_statustext(lp: THandle; statuscode: integer): PAnsiChar; stdcall;
// Create and initialise a lprec structure defaults

procedure delete_lp(lp: THandle); stdcall;
procedure free_lp(var plp: THandle); stdcall;
// Remove problem from memory

function set_lp_name(lp: THandle; lpname: PAnsiChar): boolean; stdcall;
function get_lp_name(lp: THandle): PAnsiChar; stdcall;
// Set and get the problem name

function has_BFP(lp: THandle): boolean; stdcall;
function is_nativeBFP(lp: THandle): boolean; stdcall;
function set_BFP(lp: THandle; filename: PAnsiChar): boolean; stdcall;
// Set basis factorization engine


function read_XLI(xliname, modelname, dataname, options: PAnsiChar; verbose: integer): THandle; stdcall;
function write_XLI(lp: THandle; filename, options: PAnsiChar; results: boolean): boolean; stdcall;
function has_XLI(lp: THandle): boolean; stdcall;
function is_nativeXLI(lp: THandle): boolean; stdcall;
function set_XLI(lp: THandle; filename: PAnsiChar): boolean; stdcall;
// Set external language interface

function set_obj(lp: THandle; Column: integer; Value: double): boolean; stdcall;
function set_obj_fn(lp: THandle; row: PFloatArray): boolean; stdcall;
function set_obj_fnex(lp: THandle; count: integer; row: PFloatArray;
  colno: PIntArray): boolean; stdcall;
// set the objective function (Row 0) of the matrix
function str_set_obj_fn(lp: THandle; row_string: PAnsiChar): boolean; stdcall;
// The same, but with string input
procedure set_sense(lp: THandle; maximize: boolean); stdcall;
procedure set_maxim(lp: THandle); stdcall;
procedure set_minim(lp: THandle); stdcall;
function is_maxim(lp: THandle): boolean; stdcall;
// Set optimization direction for the objective function

function add_constraint(lp: THandle; row: PFloatArray; constr_type: integer; rh: double): boolean; stdcall;
function add_constraintex(lp: THandle; count: integer; row: PFloatArray; colno: PIntArray;
  constr_type: integer; rh: double): boolean; stdcall;
function set_add_rowmode(lp: THandle; turnon: boolean): boolean; stdcall;
function is_add_rowmode(lp: THandle): boolean; stdcall;
{ Add a constraint to the problem, row is the constraint row, rh is the right hand side,
   constr_type is the type of constraint (LE (<=), GE(>=), EQ(=)) }
function str_add_constraint(lp: THandle; row_string : PAnsiChar; constr_type: integer; rh: double): boolean; stdcall;
// The same, but with string input

function set_row(lp: THandle; row_no: Integer; row: PFloatArray): boolean; stdcall;
function set_rowex(lp: THandle; row_no, count: Integer; row: PFloatArray; colno: PIntArray): boolean; stdcall;

function get_row(lp: THandle; row_nr: integer; row: PFloatArray): boolean; stdcall;
// Fill row with the row row_nr from the problem

function del_constraint(lp: THandle; del_row: integer): boolean; stdcall;
// Remove constrain nr del_row from the problem

function add_lag_con(lp: THandle; row: PFloatArray; con_type: integer; rhs: double): boolean; stdcall;
// add a Lagrangian constraint of form Row' x contype Rhs
function str_add_lag_con(lp: THandle; row_string: PAnsiChar; con_type: integer; rhs: double): boolean; stdcall;
// The same, but with string input
procedure set_lag_trace(lp: THandle; lag_trace: boolean); stdcall;
function is_lag_trace(lp: THandle): boolean; stdcall;
// Set debugging/tracing mode of the Lagrangean solver

function set_constr_type(lp: THandle; row: integer; con_type: integer): boolean; stdcall;
function get_constr_type(lp: THandle; row: integer): integer; stdcall;
function is_constr_type(lp: THandle; row: integer; mask: integer): boolean; stdcall;
// Set the type of constraint in row Row (LE, GE, EQ)

function set_rh(lp: THandle; row: integer; value: double): boolean; stdcall;
function get_rh(lp: THandle; row: integer): double; stdcall;
// Set and get the right hand side of a constraint row
function set_rh_range(lp: THandle; row: integer; deltavalue: double): boolean; stdcall;
function get_rh_range(lp: THandle; row: integer): double; stdcall;
// Set the RHS range; i.e. the lower and upper bounds of a constraint row
procedure set_rh_vec(lp: THandle; rh: PFloatArray); stdcall;
// Set the right hand side vector
function str_set_rh_vec(lp: THandle; rh_string: PAnsiChar): boolean; stdcall;
// The same, but with string input

function add_column(lp: THandle; column: PFloatArray): boolean; stdcall;
function add_columnex(lp: THandle; count: integer; column: PFloatArray; rowno: PIntArray): boolean; stdcall;
// Add a column to the problem
function str_add_column(lp: THandle; col_string: PAnsiChar): boolean; stdcall;
// The same, but with string input
function set_column(lp: THandle; col_no: Integer; column: PFloatArray): boolean; stdcall;
function set_columnex(lp: THandle; col_no, count: Integer; column: PFloatArray; rowno: PIntArray): boolean; stdcall;

function column_in_lp(lp: THandle; column: PFloatArray): integer; stdcall;
{ Returns the column index if column is already present in lp, otherwise 0.
   (Does not look at bounds and types, only looks at matrix values }
function get_column(lp: THandle; col_nr: integer; column: PFloatArray): boolean; stdcall;
// Fill column with the column col_nr from the problem

function del_column(lp: THandle; column: integer): boolean; stdcall;
// Delete a column

function set_mat(lp: THandle; row: integer; column: integer; value: double): boolean; stdcall;
{ Fill in element (Row,Column) of the matrix
   Row in [0..Rows] and Column in [1..Columns] }
function get_mat(lp: THandle; row: integer; column: integer): double; stdcall;
function get_mat_byindex(lp: THandle; matindex: Integer; isrow, adjustsign: boolean): double; stdcall;
function get_nonzeros(lp: THandle): integer; stdcall;

procedure set_bounds_tighter(lp: THandle; tighten: boolean); stdcall;
function get_bounds_tighter(lp: THandle): boolean; stdcall;
function set_upbo(lp: THandle; column: integer; value: double): boolean; stdcall;
function get_upbo(lp: THandle; column: integer): double; stdcall;
function set_lowbo(lp: THandle; column: integer; value: double): boolean; stdcall;
function get_lowbo(lp: THandle; column: integer): double; stdcall;
function set_bounds(lp: THandle; column: integer; lower: double; upper: double): boolean; stdcall;
//function get_bounds(lp: THandle; column: Integer; lower, upper: PFloatArray): boolean; stdcall;

function set_int(lp: THandle; column: integer; must_be_int: boolean): boolean; stdcall;
function is_int(lp: THandle; column: integer): boolean; stdcall;
function set_binary(lp: THandle; column: integer; must_be_bin: boolean): boolean; stdcall;
function is_binary(lp: THandle; column: integer): boolean; stdcall;
function set_semicont(lp: THandle; column: integer; must_be_sc: boolean): boolean; stdcall;
function is_semicont(lp: THandle; column: integer): boolean; stdcall;
function is_negative(lp: THandle; column: integer): boolean; stdcall;
function set_var_weights(lp: THandle; weights: PFloatArray): boolean; stdcall;
function get_var_priority(lp: THandle; column: integer): integer; stdcall;
// Set the type of variable, if must_be_int = TRUE then the variable must be integer

function add_SOS(lp: THandle; name: PAnsiChar; sostype: integer; priority: integer;
  count: integer; sosvars: PIntArray; weights: PFloatArray): integer; stdcall;
function is_SOS_var(lp: THandle; column: integer): boolean; stdcall;
// Add SOS constraints

function set_row_name(lp: THandle; row: integer; new_name: PAnsiChar): boolean; stdcall;
function get_row_name(lp: THandle; row: integer): PAnsiChar; stdcall;
function get_origrow_name(lp: THandle; row: integer): PAnsiChar; stdcall;
// Set/Get the name of a constraint row - Get added by KE

function set_col_name(lp: THandle; column: integer; new_name: PAnsiChar): boolean; stdcall;
function get_col_name(lp: THandle; column: integer): PAnsiChar; stdcall;
function get_origcol_name(lp: THandle; column: integer): PAnsiChar; stdcall;
// Set/Get the name of a variable column - Get added by KE

procedure unscale(lp: THandle); stdcall;
// Undo previous scaling of the problem

procedure set_preferdual(lp: THandle; dodual: boolean); stdcall;
procedure set_simplextype(lp: THandle; simplextype: integer); stdcall;
function get_simplextype(lp: THandle): integer; stdcall;
// Set/Get if lp_solve should prefer the dual simplex over the primal -- added by KE

procedure default_basis(lp: THandle); stdcall;
procedure set_basiscrash(lp: THandle; mode: integer); stdcall;
function get_basiscrash(lp: THandle): integer; stdcall;
function set_basis(lp: THandle; bascolumn: PIntArray; nonbasic: boolean): boolean; stdcall;

function is_feasible(lp: THandle; values: PFloatArray; threshold: double): boolean; stdcall;
// returns TRUE if the vector in values is a feasible solution to the lp

function solve(lp: THandle): integer; stdcall;
// Solve the problem

function time_elapsed(lp: THandle): double; stdcall;
// Return the number of seconds since start of solution process

function get_primal_solution(lp: THandle; pv: PFloatArray): boolean; stdcall;
function get_ptr_primal_solution(lp: THandle; var pv: PFloatArray): boolean; stdcall;
function get_dual_solution(lp: THandle; rc: PFloatArray): boolean; stdcall;
function get_ptr_dual_solution(lp: THandle; var rc: PFloatArray): boolean; stdcall;
function get_lambda(lp: THandle; lambda: PFloatArray): boolean; stdcall;
function get_ptr_lambda(lp: THandle; var lambda: PFloatArray): boolean; stdcall;
// Get the primal, dual/reduced costs and Lambda vectors

procedure reset_basis(lp: THandle); stdcall;
// Reset the basis of a problem, can be useful in case of degeneracy - JD

// Read an MPS file
function read_MPS(filename: PAnsiChar; options: integer): THandle; stdcall; overload;
function read_mps(stream: PInteger; options: integer): THandle; stdcall; overload;
// Write a MPS file to output
function write_mps(lp: THandle; filename: PAnsiChar): boolean; stdcall; overload;
function write_MPS(lp: THandle; output: PInteger): boolean; stdcall; overload;


function read_freeMPS(filename: PAnsiChar; options: Integer): THandle; stdcall; overload;
function read_freemps(filename: Pinteger; options: Integer): THandle; stdcall; overload;
function write_freemps(lp: THandle; filename: PAnsiChar): boolean; stdcall; overload;
function write_freeMPS(lp: THandle; output: PInteger): boolean; stdcall; overload;

function guess_basis(lp: THandle; guessvector: PFloatArray; basisvector: PIntArray): boolean; stdcall;
function read_basis(lp: THandle; filename, info: PAnsiChar): boolean; stdcall;
function write_basis(lp: THandle; filename: PAnsiChar): boolean; stdcall;

function write_lp(lp: THandle; filename: PAnsiChar): boolean; stdcall; overload;
function write_LP(lp: THandle; filename: PInteger): boolean; stdcall; overload;
// Write a LP file to output

function read_lp(filename: Pinteger; verbose: integer; lp_name: PAnsiChar): THandle; stdcall; overload;
function read_LP(filename: PAnsiChar; verbose: integer; lp_name: PAnsiChar): THandle; stdcall; overload;
// Old-style lp format file parser

procedure print_lp(lp: THandle); stdcall;
procedure print_tableau(lp: THandle); stdcall;
// Print the current problem, only useful in very small (test) problems

procedure print_objective(lp: THandle); stdcall;
procedure print_solution(lp: THandle; columns: integer); stdcall;
procedure print_constraints(lp: THandle; columns: integer); stdcall;
// Print the solution to stdout

procedure print_duals(lp: THandle); stdcall;
// Print the dual variables of the solution

procedure print_scales(lp: THandle); stdcall;
// If scaling is used, print the scaling factors

procedure print_str(lp: THandle; str: PAnsiChar); stdcall;

procedure set_outputstream(lp: THandle; stream: Pointer); stdcall;
function set_outputfile(lp: THandle; filename: PAnsiChar): boolean; stdcall;

procedure set_verbose(lp: THandle; verbose: integer); stdcall;
function get_verbose(lp: THandle): integer; stdcall;

procedure set_timeout(lp: THandle; sectimeout: integer); stdcall;
function get_timeout(lp: THandle): integer; stdcall;

procedure set_print_sol(lp: THandle; print_sol: integer); stdcall;
function get_print_sol(lp: THandle): integer; stdcall;

procedure set_debug(lp: THandle; debug: boolean); stdcall;
function is_debug(lp: THandle): boolean; stdcall;

procedure set_trace(lp: THandle; trace: boolean); stdcall;
function is_trace(lp: THandle): boolean; stdcall;

function print_debugdump(lp: THandle; filename: PAnsiChar): boolean; stdcall;

procedure set_anti_degen(lp: THandle; anti_degen: integer); stdcall;
function get_anti_degen(lp: THandle): integer; stdcall;
function is_anti_degen(lp: THandle; testmask: integer): boolean; stdcall;

function get_presolve(lp: THandle): integer; stdcall;
function is_presolve(lp: THandle; testmask: integer): boolean; stdcall;

function get_orig_index(lp: THandle; lp_index: integer): integer; stdcall;
function get_lp_index(lp: THandle; orig_index: integer): integer; stdcall;

procedure set_maxpivot(lp: THandle; max_num_inv: integer); stdcall;
function get_maxpivot(lp: THandle): integer; stdcall;

procedure set_obj_bound(lp: THandle; obj_bound: double); stdcall;
function get_obj_bound(lp: THandle): double; stdcall;

procedure set_mip_gap(lp: THandle; absolute: boolean; mip_gap: double); stdcall;
function get_mip_gap(lp: THandle; absolute: boolean): double; stdcall;

procedure set_bb_rule(lp: THandle; bb_rule: integer); stdcall;
function get_bb_rule(lp: THandle): integer; stdcall;

function set_var_branch(lp: THandle; column: integer; branch_mode: integer): boolean; stdcall;
function get_var_branch(lp: THandle; column: integer): integer; stdcall;

function is_infinite(lp: THandle; value: double): boolean; stdcall;
procedure set_infinite(lp: THandle; infinite: double); stdcall;
function get_infinite(lp: THandle): double; stdcall;

procedure set_epsint(lp: THandle; epsilon: double); stdcall;
function get_epsint(lp: THandle): double; stdcall;

procedure set_epsb(lp: THandle; epsb: double); stdcall;
function get_epsb(lp: THandle): double; stdcall;

procedure set_epsd(lp: THandle; epsd: double); stdcall;
function get_epsd(lp: THandle): double; stdcall;

procedure set_epsel(lp: THandle; epsel: double); stdcall;
function get_epsel(lp: THandle): double; stdcall;

procedure set_scaling(lp: THandle; scalemode: integer); stdcall;
function get_scaling(lp: THandle): integer; stdcall;
function is_scalemode(lp: THandle; testmask: integer): boolean; stdcall;
function is_scaletype(lp: THandle; scaletype: integer): boolean; stdcall;
function is_integerscaling(lp: THandle): boolean; stdcall;
procedure set_scalelimit(lp: THandle; scalelimit: double); stdcall;
function get_scalelimit(lp: THandle): double; stdcall;

procedure set_improve(lp: THandle; improve: integer); stdcall;
function get_improve(lp: THandle): integer; stdcall;

procedure set_pivoting(lp: THandle; piv_rule: integer); stdcall;
function get_pivoting(lp: THandle): integer; stdcall;

function set_partialprice(lp: THandle; blockcount: Integer; blockstart: PIntArray; isrow: boolean): boolean; stdcall;
procedure get_partialprice(lp: THandle; blockcount: PIntArray; blockstart: PIntArray; isrow: boolean); stdcall;

function set_multiprice(lp: THandle; multiblockdiv: Integer): boolean; stdcall;
function get_multiprice(lp: THandle; getabssize: boolean): Integer; stdcall;

function is_piv_mode(lp: THandle; testmask: integer): boolean; stdcall;
function is_piv_rule(lp: THandle; rule: integer): boolean; stdcall;

procedure set_break_at_first(lp: THandle; break_at_first: boolean); stdcall;
function is_break_at_first(lp: THandle): boolean; stdcall;

procedure set_bb_floorfirst(lp: THandle; bb_floorfirst: integer); stdcall;
function get_bb_floorfirst(lp: THandle): integer; stdcall;

procedure set_bb_depthlimit(lp: THandle; bb_maxlevel: integer); stdcall;
function get_bb_depthlimit(lp: THandle): integer; stdcall;

procedure set_break_at_value(lp: THandle; break_at_value: double); stdcall;
function get_break_at_value(lp: THandle): double; stdcall;

procedure set_negrange(lp: THandle; negrange: double); stdcall;
function get_negrange(lp: THandle): double; stdcall;

procedure set_epsperturb(lp: THandle; epsperturb: double); stdcall;
function get_epsperturb(lp: THandle): double; stdcall;

procedure set_epspivot(lp: THandle; epspivot: double); stdcall;
function get_epspivot(lp: THandle): double; stdcall;

function get_max_level(lp: THandle): integer; stdcall;
function get_total_nodes(lp: THandle): COUNTER; stdcall;
function get_total_iter(lp: THandle): COUNTER; stdcall;

function get_objective(lp: THandle): double; stdcall;
function get_working_objective(lp: THandle): double; stdcall;

function get_var_primalresult(lp: THandle; index: integer): double; stdcall;
function get_var_dualresult(lp: THandle; index: integer): double; stdcall;

function get_variables(lp: THandle; var_: PFloatArray): boolean; stdcall;
function get_ptr_variables(lp: THandle; var var_: PFloatArray): boolean; stdcall;

function get_constraints(lp: THandle; constr: PFloatArray): boolean; stdcall;
function get_ptr_constraints(lp: THandle; var constr: PFloatArray): boolean; stdcall;

function get_sensitivity_rhs(lp: THandle; duals, dualsfrom, dualstill: PFloatArray): boolean; stdcall;
function get_ptr_sensitivity_rhs(lp: THandle; var duals, dualsfrom, dualstill: PFloatArray): boolean; stdcall;

function get_sensitivity_obj(lp: THandle; objfrom, objtill: PFloatArray): boolean; stdcall;
function get_sensitivity_objex(lp: THandle; objfrom, objtill, objfromvalue, objtillvalue: PFloatArray): boolean; stdcall;
function get_ptr_sensitivity_obj(lp: THandle; var objfrom, objtill: PFloatArray): boolean; stdcall;
function get_ptr_sensitivity_objex(lp: THandle; var objfrom, objtill, objfromvalue, objtillvalue: PFloatArray): boolean; stdcall;


procedure set_solutionlimit(lp: THandle; limit: integer); stdcall;
function get_solutionlimit(lp: THandle): integer; stdcall;
function get_solutioncount(lp: THandle): integer; stdcall;

function get_Norig_rows(lp: THandle): integer; stdcall;
function get_Nrows(lp: THandle): integer; stdcall;
function get_Lrows(lp: THandle): integer; stdcall;

function get_Norig_columns(lp: THandle): integer; stdcall;
function get_Ncolumns(lp: THandle): integer; stdcall;

function get_nameindex(lp: THandle; varname: PAnsiChar; isrow: boolean): Integer; stdcall;


{$IFDEF LPS55_UP}
function copy_lp(lp: THandle): THandle; stdcall;
function dualize_lp(lp: THandle): boolean; stdcall;
(* Copy or dualize the lp *)
function get_constr_value(lp: THandle; rownr, count: Integer;
  primsolution: PFloatArray; nzindex: PIntArray): double; stdcall;
function get_columnex(lp: THandle; colnr: Integer; column: PFloatArray;
  nzrow: PIntArray): Integer; stdcall;
function set_unbounded(lp: THandle; colnr: Integer): boolean; stdcall;
function is_unbounded(lp: THandle; colnr: Integer): boolean; stdcall;
function get_basis(lp: THandle; bascolumn: PIntArray; nonbasic: boolean): boolean; stdcall;
function set_basisvar(lp: THandle; basisPos, enteringCol: Integer): Integer; stdcall;
procedure put_bb_nodefunc(lp: THandle; newnode: lphandleint_intfunc; bbnodehandle: Pointer); stdcall;
procedure put_bb_branchfunc(lp: THandle; newbranch: lphandleint_intfunc; bbbranchhandle: Pointer); stdcall;
// Allow the user to override B&B node and branching decisions
procedure put_abortfunc(lp: THandle; newctrlc: lphandle_intfunc; ctrlchandle: Pointer); stdcall;
// Allow the user to define an interruption callback function
procedure put_logfunc(lp: THandle; newlog: lphandlestr_func; loghandle: Pointer); stdcall;
// Allow the user to define a logging function
procedure put_msgfunc(lp: THandle; newmsg: lphandleint_func; msghandle: Pointer; mask: integer); stdcall;
// Allow the user to define an event-driven message/reporting
function write_params(lp: THandle; filename, options: PAnsiChar): boolean; stdcall;
function read_params(lp: THandle; filename, options: PAnsiChar): boolean; stdcall;
procedure reset_params(lp: THandle); stdcall;
// Read and write parameter file
procedure set_presolve(lp: THandle; presolvemode, maxloops: Integer); stdcall;
function get_presolveloops(lp: THandle): Integer; stdcall;
function set_epslevel(lp: THandle; epslevel: Integer): boolean; stdcall;
function get_rowex(lp: THandle; rownr: Integer; row: PFloatArray; colno: PIntArray): integer; stdcall;
function is_use_names(lp: THandle; isrow: boolean): boolean; stdcall;
procedure set_use_names(lp: THandle; isrow, use_names: boolean); stdcall;
function is_obj_in_basis(lp: THandle): boolean; stdcall;
procedure set_obj_in_basis(lp: THandle; obj_in_basis: boolean); stdcall;

function get_accuracy(lp: THandle): double; stdcall;
function get_break_numeric_accuracy(lp: THandle): double; stdcall;
procedure set_break_numeric_accuracy(lp: THandle;Value: double); stdcall;

{$ELSE}
function set_free(lp: THandle; column: integer): boolean; stdcall;
function is_free(lp: THandle; column: integer): boolean; stdcall;
// Set the upper and lower bounds of a variable
procedure get_basis(lp: THandle; bascolumn: PIntArray; nonbasic: boolean); stdcall;
// Set/Get basis for a re-solved system - Added by KE
procedure put_abortfunc(lp: THandle; newctrlc: ctrlcfunc; ctrlchandle: Pointer); stdcall;
// Allow the user to define an interruption callback function
procedure put_logfunc(lp: THandle; newlog: logfunc; loghandle: Pointer); stdcall;
// Allow the user to define a logging function
procedure put_msgfunc(lp: THandle; newmsg: msgfunc; msghandle: Pointer; mask: integer); stdcall;
// Allow the user to define an event-driven message/reporting
procedure set_presolve(lp: THandle; do_presolve: integer); stdcall;
{$ENDIF}

{$IFDEF LPSOLVE_DYNAMIC}
function LoadLPSolve: Boolean;
procedure UnloadLPSolve;
function IsLPSolveLoaded: Boolean;
{$ENDIF}

implementation


const
  {$IFDEF UNIX}
    {$IFDEF LPS51}
      LPSOLVELIB = 'liblpsolve51.so';
    {$ENDIF}
    {$IFDEF LPS55}
      LPSOLVELIB = 'liblpsolve55.so';
    {$ENDIF}
  {$ELSE}
    {$IFDEF LPS51}
      LPSOLVELIB = 'lpsolve51.dll';
    {$ENDIF}
    {$IFDEF LPS55}
      LPSOLVELIB = 'lpsolve55.dll';
    {$ENDIF}
  {$ENDIF}

{$IFDEF LPSOLVE_DYNAMIC}
type
  // Generic types based on function signatures
  TProc_PInt4 = procedure(p1, p2, p3, p4: PInteger); stdcall;
  TFunc_Int2_THandle = function(i1, i2: Integer): THandle; stdcall;
  TFunc_THandle_Int_Bool = function(lp: THandle; i: Integer): Boolean; stdcall;
  TFunc_THandle_Int2_Bool = function(lp: THandle; i1, i2: Integer): Boolean; stdcall;
  TFunc_THandle_Int = function(lp: THandle): Integer; stdcall;
  TFunc_THandle_Int_PAnsiChar = function(lp: THandle; i1: Integer): PAnsiChar; stdcall;
  TFunc_THandle_Int_PAnsiChar_Bool = function(lp: THandle; i1: Integer; s: PAnsiCHar): boolean; stdcall;
  TProc_THandle = procedure(lp: THandle); stdcall;
  TProc_pTHandle = procedure(var p: THandle); stdcall;
  TFunc_THandle_PAnsiChar_Bool = function(lp: THandle; s: PAnsiChar): Boolean; stdcall;
  TFunc_THandle_PAnsiChar = function(lp: THandle): PAnsiChar; stdcall;
  TFunc_THandle_Bool = function(lp: THandle): Boolean; stdcall;
  TFunc_Str4_Int_THandle = function(s1, s2, s3, s4: PAnsiChar; i: Integer): THandle; stdcall;
  TFunc_THandle_Str2_Bool_Bool = function(lp: THandle; s1, s2: PAnsiChar; b: Boolean): Boolean; stdcall;
  TFunc_THandle_Int_Double_Bool = function(lp: THandle; i: Integer; d: Double): Boolean; stdcall;
  TFunc_THandle_Int_Double2_Bool = function(lp: THandle; i: Integer; d1, d2: Double): Boolean; stdcall;
  TFunc_THandle_PFloat_Bool = function(lp: THandle; p: PFloatArray): Boolean; stdcall;
  TFunc_THandle_Int_PFloat_PInt_Bool = function(lp: THandle; i: Integer; p1: PFloatArray; p2: PIntArray): Boolean; stdcall;
  TProc_THandle_Bool = procedure(lp: THandle; b: Boolean); stdcall;
  TFunc_THandle_PFloat_Int_Double_Bool = function(lp: THandle; p: PFloatArray; i: Integer; d: Double): Boolean; stdcall;
  TFunc_THandle_Int_PFloat_PInt_Int_Double_Bool = function(lp: THandle; i1: Integer; p1: PFloatArray; p2: PIntArray; i2: Integer; d: Double): Boolean; stdcall;
  TFunc_THandle_Str_Int_Double_Bool = function(lp: THandle; s: PAnsiChar; i: Integer; d: Double): Boolean; stdcall;
  TFunc_THandle_Int_PFloat_Bool = function(lp: THandle; i: Integer; p: PFloatArray): Boolean; stdcall;
  TFunc_THandle_PFloat_Int_Double_Bool2 = function(lp: THandle; p: PFloatArray; i: Integer; d: Double): Boolean; stdcall;
  TFunc_THandle_Str_Int_Double_Bool2 = function(lp: THandle; s: PAnsiChar; i: Integer; d: Double): Boolean; stdcall;
  TFunc_THandle_Int_Int_Bool = function(lp: THandle; i1, i2: Integer): Boolean; stdcall;
  TFunc_THandle_Int_Int = function(lp: THandle; i: Integer): Integer; stdcall;
  TFunc_THandle_Int_Double_Bool2 = function(lp: THandle; i: Integer; d: Double): Boolean; stdcall;
  TFunc_THandle_Int_Double = function(lp: THandle; i: Integer): Double; stdcall;
  TFunc_THandle_Int2_Double = function(lp: THandle; i1, i2: Integer): Double; stdcall;
  TProc_THandle_PFloat = procedure(lp: THandle; p: PFloatArray); stdcall;
  TFunc_THandle_PFloat_Int = function(lp: THandle; p: PFloatArray): Integer; stdcall;
  TFunc_THandle_Int3_Double_Bool = function(lp: THandle; i1, i2, i3: Integer; d: Double): Boolean; stdcall;
  TFunc_THandle_Int2_Double_Bool = function(lp: THandle; i1, i2: Integer; d: Double): Boolean; stdcall;
  TFunc_THandle_Double = function(lp: THandle): Double; stdcall;
  TFunc_THandle_Int_Bool_Double = function(lp: THandle; i: Integer; b: Boolean): Double; stdcall;
  TFunc_THandle_Int_Bool_Bool = function(lp: THandle; i1: Integer; b: Boolean): Boolean; stdcall;
  TFunc_THandle_Str_Int5_PInt_PFloat_Int = function(lp: THandle; s: PAnsiChar; i1, i2, i3: Integer; p1: PIntArray; p2: PFloatArray): Integer; stdcall;
  TProc_THandle_Int = procedure(lp: THandle; i: Integer); stdcall;
  TFunc_THandle_PInt_Bool_Bool = function(lp: THandle; p: PIntArray; b: Boolean): Boolean; stdcall;
  TFunc_THandle_PFloat_Double_Bool = function(lp: THandle; p: PFloatArray; d: Double): Boolean; stdcall;
  TFunc_THandle_pPFloat_Bool = function(lp: THandle; var p: PFloatArray): Boolean; stdcall;
  TFunc_Str_Int_THandle = function(s: PAnsiChar; i: Integer): THandle; stdcall;
  TFunc_PInt_Int_THandle = function(p: PInteger; i: Integer): THandle; stdcall;
  TFunc_THandle_PInt_Bool = function(lp: THandle; p: PInteger): Boolean; stdcall;
  TFunc_THandle_PFloat_PInt_Bool = function(lp: THandle; p1: PFloatArray; p2: PIntArray): Boolean; stdcall;
  TFunc_THandle_Str2_Bool = function(lp: THandle; s1, s2: PAnsiChar): Boolean; stdcall;
  TFunc_PInt_Int_PAnsiChar_THandle = function(p: PInteger; i: Integer; s: PAnsiChar): THandle; stdcall;
  TFunc_Str_Int_PAnsiChar_THandle = function(s1: PAnsiChar; i: Integer; s2: PAnsiChar): THandle; stdcall;
  TProc_THandle_Pointer = procedure(lp: THandle; p: Pointer); stdcall;
  TProc_THandle_Double = procedure(lp: THandle; d: Double); stdcall;
  TProc_THandle_Bool_Double = procedure(lp: THandle; b: Boolean; d: Double); stdcall;
  TFunc_THandle_Bool_Double = function(lp: THandle; b: Boolean): Double; stdcall;
  TFunc_THandle_Double_Bool = function(lp: THandle; d: Double): Boolean; stdcall;
  TFunc_THandle_COUNTER = function(lp: THandle): COUNTER; stdcall;
  TFunc_THandle_Int_Double2 = function(lp: THandle; i: Integer): Double; stdcall;
  TFunc_THandle_PFloat3_Bool = function(lp: THandle; p1, p2, p3: PFloatArray): Boolean; stdcall;
  TFunc_THandle_pPFloat3_Bool = function(lp: THandle; var p1, p2, p3: PFloatArray): Boolean; stdcall;
  TFunc_THandle_PFloat2_Bool = function(lp: THandle; p1, p2: PFloatArray): Boolean; stdcall;
  TFunc_THandle_PFloat4_Bool = function(lp: THandle; p1, p2, p3, p4: PFloatArray): Boolean; stdcall;
  TFunc_THandle_pPFloat2_Bool = function(lp: THandle; var p1, p2: PFloatArray): Boolean; stdcall;
  TFunc_THandle_pPFloat4_Bool = function(lp: THandle; var p1, p2, p3, p4: PFloatArray): Boolean; stdcall;
  TFunc_THandle_Str_Bool_Int = function(lp: THandle; s: PAnsiChar; b: Boolean): Integer; stdcall;
  TFunc_THandle_Int_PInt_Bool_Bool = function(lp: THandle; i: Integer; p: PIntArray; b: Boolean): Boolean; stdcall;
  TProc_THandle_PInt2_Bool = procedure(lp: THandle; p1, p2: PIntArray; b: Boolean); stdcall;
  TFunc_THandle_Bool_Int = function(lp: THandle; b: Boolean): Integer; stdcall;
  TFunc_THandle_THandle = function(lp: THandle): THandle; stdcall;
  TFunc_THandle_Int2_PFloat_PInt_Double = function(lp: THandle; i1, i2: Integer; p1: PFloatArray; p2: PIntArray): Double; stdcall;
  TFunc_THandle_Int2_PFloat_PInt_Bool = function(lp: THandle; i1, i2: Integer; p1: PFloatArray; p2: PIntArray): boolean; stdcall;
  TFunc_THandle_Int_PFloat_PInt_Int = function(lp: THandle; i1: Integer; p1: PFloatArray; p2: PIntArray): Integer; stdcall;
  TFunc_THandle_Int_Bool2 = function(lp: THandle; i: Integer): Boolean; stdcall;
  TFunc_THandle_Int2_Int = function(lp: THandle; i1, i2: Integer): Integer; stdcall;
  {$IFDEF LPS55_UP}
  TProc_THandle_lphandleint_intfunc_Pointer = procedure(lp: THandle; f: lphandleint_intfunc; p: Pointer); stdcall;
  TProc_THandle_lphandle_intfunc_Pointer = procedure(lp: THandle; f: lphandle_intfunc; p: Pointer); stdcall;
  TProc_THandle_lphandlestr_func_Pointer = procedure(lp: THandle; f: lphandlestr_func; p: Pointer); stdcall;
  TProc_THandle_lphandleint_func_Pointer_Int = procedure(lp: THandle; f: lphandleint_func; p: Pointer; i: Integer); stdcall;
  {$ELSE}
  TProc_THandle_ctrlcfunc_Pointer = procedure(lp: THandle; f: ctrlcfunc; p: Pointer); stdcall;
  TProc_THandle_logfunc_Pointer = procedure(lp: THandle; f: logfunc; p: Pointer); stdcall;
  TProc_THandle_msgfunc_Pointer_Int = procedure(lp: THandle; f: msgfunc; p: Pointer; i: Integer); stdcall;
  {$ENDIF}
  TProc_THandle_Int2 = procedure(lp: THandle; i1, i2: Integer); stdcall;
  TFunc_THandle_Bool_Bool = function(lp: THandle; b: Boolean): Boolean; stdcall;
  TFunc_THandle_Bool2_Bool = function(lp: THandle; b1, b2: Boolean): Boolean; stdcall;
  TProc_THandle_Bool2 = procedure(lp: THandle; b1, b2: Boolean); stdcall;

var
  FLibHandle: THandle = 0;
  // Function Pointers
  Flp_solve_version: TProc_PInt4;
  Fmake_lp: TFunc_Int2_THandle;
  Fresize_lp: TFunc_THandle_Int2_Bool;
  Fget_status: TFunc_THandle_Int;
  Fget_statustext: TFunc_THandle_Int_PAnsiChar;
  Fdelete_lp: TProc_THandle;
  Ffree_lp: TProc_pTHandle;
  Fset_lp_name: TFunc_THandle_PAnsiChar_Bool;
  Fget_lp_name: TFunc_THandle_PAnsiChar;
  Fhas_BFP: TFunc_THandle_Bool;
  Fis_nativeBFP: TFunc_THandle_Bool;
  Fset_BFP: TFunc_THandle_PAnsiChar_Bool;
  Fread_XLI: TFunc_Str4_Int_THandle;
  Fwrite_XLI: TFunc_THandle_Str2_Bool_Bool;
  Fhas_XLI: TFunc_THandle_Bool;
  Fis_nativeXLI: TFunc_THandle_Bool;
  Fset_XLI: TFunc_THandle_PAnsiChar_Bool;
  Fset_obj: TFunc_THandle_Int_Double_Bool;
  Fset_obj_fn: TFunc_THandle_PFloat_Bool;
  Fset_obj_fnex: TFunc_THandle_Int_PFloat_PInt_Bool;
  Fstr_set_obj_fn: TFunc_THandle_PAnsiChar_Bool;
  Fset_sense: TProc_THandle_Bool;
  Fset_maxim: TProc_THandle;
  Fset_minim: TProc_THandle;
  Fis_maxim: TFunc_THandle_Bool;
  Fadd_constraint: TFunc_THandle_PFloat_Int_Double_Bool;
  Fadd_constraintex: TFunc_THandle_Int_PFloat_PInt_Int_Double_Bool;
  Fset_add_rowmode: TFunc_THandle_Bool_Bool;
  Fis_add_rowmode: TFunc_THandle_Bool;
  Fstr_add_constraint: TFunc_THandle_Str_Int_Double_Bool;
  Fset_row: TFunc_THandle_Int_PFloat_Bool;
  Fset_rowex: TFunc_THandle_Int2_PFloat_PInt_Bool;
  Fget_row: TFunc_THandle_Int_PFloat_Bool;
  Fdel_constraint: TFunc_THandle_Int_Bool;
  Fadd_lag_con: TFunc_THandle_PFloat_Int_Double_Bool2;
  Fstr_add_lag_con: TFunc_THandle_Str_Int_Double_Bool2;
  Fset_lag_trace: TProc_THandle_Bool;
  Fis_lag_trace: TFunc_THandle_Bool;
  Fset_constr_type: TFunc_THandle_Int_Int_Bool;
  Fget_constr_type: TFunc_THandle_Int_Int;
  Fis_constr_type: TFunc_THandle_Int_Int_Bool;
  Fset_rh: TFunc_THandle_Int_Double_Bool2;
  Fget_rh: TFunc_THandle_Int_Double;
  Fset_rh_range: TFunc_THandle_Int_Double_Bool2;
  Fget_rh_range: TFunc_THandle_Int_Double;
  Fset_rh_vec: TProc_THandle_PFloat;
  Fstr_set_rh_vec: TFunc_THandle_PAnsiChar_Bool;
  Fadd_column: TFunc_THandle_PFloat_Bool;
  Fadd_columnex: TFunc_THandle_Int_PFloat_PInt_Bool;
  Fstr_add_column: TFunc_THandle_PAnsiChar_Bool;
  Fset_column: TFunc_THandle_Int_PFloat_Bool;
  Fset_columnex: TFunc_THandle_Int2_PFloat_PInt_Bool;
  Fcolumn_in_lp: TFunc_THandle_PFloat_Int;
  Fget_column: TFunc_THandle_Int_PFloat_Bool;
  Fdel_column: TFunc_THandle_Int_Bool;
  Fset_mat: TFunc_THandle_Int2_Double_Bool;
  Fget_mat: TFunc_THandle_Int2_Double;
  Fget_mat_byindex: function(lp: THandle; matindex: Integer; isrow, adjustsign: boolean): double; stdcall;
  Fget_nonzeros: TFunc_THandle_Int;
  Fset_bounds_tighter: TProc_THandle_Bool;
  Fget_bounds_tighter: TFunc_THandle_Bool;
  Fset_upbo: TFunc_THandle_Int_Double_Bool2;
  Fget_upbo: TFunc_THandle_Int_Double;
  Fset_lowbo: TFunc_THandle_Int_Double_Bool2;
  Fget_lowbo: TFunc_THandle_Int_Double;
  Fset_bounds: TFunc_THandle_Int_Double2_Bool;
  Fset_int: TFunc_THandle_Int_Bool_Bool;
  Fis_int: TFunc_THandle_Int_Bool;
  Fset_binary: TFunc_THandle_Int_Bool_Bool;
  Fis_binary: TFunc_THandle_Int_Bool;
  Fset_semicont: TFunc_THandle_Int_Bool_Bool;
  Fis_semicont: TFunc_THandle_Int_Bool;
  Fis_negative: TFunc_THandle_Int_Bool;
  Fset_var_weights: TFunc_THandle_PFloat_Bool;
  Fget_var_priority: TFunc_THandle_Int_Int;
  Fadd_SOS: TFunc_THandle_Str_Int5_PInt_PFloat_Int;
  Fis_SOS_var: TFunc_THandle_Int_Bool;
  Fset_row_name: TFunc_THandle_Int_PAnsiChar_Bool;
  Fget_row_name: TFunc_THandle_Int_PAnsiChar;
  Fget_origrow_name: TFunc_THandle_Int_PAnsiChar;
  Fset_col_name: TFunc_THandle_Int_PAnsiChar_Bool;
  Fget_col_name: TFunc_THandle_Int_PAnsiChar;
  Fget_origcol_name: TFunc_THandle_Int_PAnsiChar;
  Funscale: TProc_THandle;
  Fset_preferdual: TProc_THandle_Bool;
  Fset_simplextype: TProc_THandle_Int;
  Fget_simplextype: TFunc_THandle_Int;
  Fdefault_basis: TProc_THandle;
  Fset_basiscrash: TProc_THandle_Int;
  Fget_basiscrash: TFunc_THandle_Int;
  Fset_basis: TFunc_THandle_PInt_Bool_Bool;
  Fis_feasible: TFunc_THandle_PFloat_Double_Bool;
  Fsolve: TFunc_THandle_Int;
  Ftime_elapsed: TFunc_THandle_Double;
  Fget_primal_solution: TFunc_THandle_PFloat_Bool;
  Fget_ptr_primal_solution: TFunc_THandle_pPFloat_Bool;
  Fget_dual_solution: TFunc_THandle_PFloat_Bool;
  Fget_ptr_dual_solution: TFunc_THandle_pPFloat_Bool;
  Fget_lambda: TFunc_THandle_PFloat_Bool;
  Fget_ptr_lambda: TFunc_THandle_pPFloat_Bool;
  Freset_basis: TProc_THandle;
  Fread_MPS_file: TFunc_Str_Int_THandle;
  Fread_mps_stream: TFunc_PInt_Int_THandle;
  Fwrite_mps_file: TFunc_THandle_PAnsiChar_Bool;
  Fwrite_MPS_stream: TFunc_THandle_PInt_Bool;
  Fread_freeMPS_file: TFunc_Str_Int_THandle;
  Fread_freemps_stream: TFunc_PInt_Int_THandle;
  Fwrite_freemps_file: TFunc_THandle_PAnsiChar_Bool;
  Fwrite_freeMPS_stream: TFunc_THandle_PInt_Bool;
  Fguess_basis: TFunc_THandle_PFloat_PInt_Bool;
  Fread_basis: TFunc_THandle_Str2_Bool;
  Fwrite_basis: TFunc_THandle_PAnsiChar_Bool;
  Fwrite_lp_file: TFunc_THandle_PAnsiChar_Bool;
  Fwrite_LP_stream: TFunc_THandle_PInt_Bool;
  Fread_lp_stream: TFunc_PInt_Int_PAnsiChar_THandle;
  Fread_LP_file: TFunc_Str_Int_PAnsiChar_THandle;
  Fprint_lp: TProc_THandle;
  Fprint_tableau: TProc_THandle;
  Fprint_objective: TProc_THandle;
  Fprint_solution: TProc_THandle_Int;
  Fprint_constraints: TProc_THandle_Int;
  Fprint_duals: TProc_THandle;
  Fprint_scales: TProc_THandle;
  Fprint_str: procedure(lp: THandle; str: PAnsiChar); stdcall;
  Fset_outputstream: TProc_THandle_Pointer;
  Fset_outputfile: TFunc_THandle_PAnsiChar_Bool;
  Fset_verbose: TProc_THandle_Int;
  Fget_verbose: TFunc_THandle_Int;
  Fset_timeout: TProc_THandle_Int;
  Fget_timeout: TFunc_THandle_Int;
  Fset_print_sol: TProc_THandle_Int;
  Fget_print_sol: TFunc_THandle_Int;
  Fset_debug: TProc_THandle_Bool;
  Fis_debug: TFunc_THandle_Bool;
  Fset_trace: TProc_THandle_Bool;
  Fis_trace: TFunc_THandle_Bool;
  Fprint_debugdump: TFunc_THandle_PAnsiChar_Bool;
  Fset_anti_degen: TProc_THandle_Int;
  Fget_anti_degen: TFunc_THandle_Int;
  Fis_anti_degen: TFunc_THandle_Int_Bool;
  Fget_presolve: TFunc_THandle_Int;
  Fis_presolve: TFunc_THandle_Int_Bool;
  Fget_orig_index: TFunc_THandle_Int_Int;
  Fget_lp_index: TFunc_THandle_Int_Int;
  Fset_maxpivot: TProc_THandle_Int;
  Fget_maxpivot: TFunc_THandle_Int;
  Fset_obj_bound: TProc_THandle_Double;
  Fget_obj_bound: TFunc_THandle_Double;
  Fset_mip_gap: TProc_THandle_Bool_Double;
  Fget_mip_gap: TFunc_THandle_Bool_Double;
  Fset_bb_rule: TProc_THandle_Int;
  Fget_bb_rule: TFunc_THandle_Int;
  Fset_var_branch: TFunc_THandle_Int_Int_Bool;
  Fget_var_branch: TFunc_THandle_Int_Int;
  Fis_infinite: TFunc_THandle_Double_Bool;
  Fset_infinite: TProc_THandle_Double;
  Fget_infinite: TFunc_THandle_Double;
  Fset_epsint: TProc_THandle_Double;
  Fget_epsint: TFunc_THandle_Double;
  Fset_epsb: TProc_THandle_Double;
  Fget_epsb: TFunc_THandle_Double;
  Fset_epsd: TProc_THandle_Double;
  Fget_epsd: TFunc_THandle_Double;
  Fset_epsel: TProc_THandle_Double;
  Fget_epsel: TFunc_THandle_Double;
  Fset_scaling: TProc_THandle_Int;
  Fget_scaling: TFunc_THandle_Int;
  Fis_scalemode: TFunc_THandle_Int_Bool;
  Fis_scaletype: TFunc_THandle_Int_Bool;
  Fis_integerscaling: TFunc_THandle_Bool;
  Fset_scalelimit: TProc_THandle_Double;
  Fget_scalelimit: TFunc_THandle_Double;
  Fset_improve: TProc_THandle_Int;
  Fget_improve: TFunc_THandle_Int;
  Fset_pivoting: TProc_THandle_Int;
  Fget_pivoting: TFunc_THandle_Int;
  Fset_partialprice: TFunc_THandle_Int_PInt_Bool_Bool;
  Fget_partialprice: TProc_THandle_PInt2_Bool;
  Fset_multiprice: TFunc_THandle_Int_Bool2;
  Fget_multiprice: TFunc_THandle_Bool_Int;
  Fis_piv_mode: TFunc_THandle_Int_Bool;
  Fis_piv_rule: TFunc_THandle_Int_Bool;
  Fset_break_at_first: TProc_THandle_Bool;
  Fis_break_at_first: TFunc_THandle_Bool;
  Fset_bb_floorfirst: TProc_THandle_Int;
  Fget_bb_floorfirst: TFunc_THandle_Int;
  Fset_bb_depthlimit: TProc_THandle_Int;
  Fget_bb_depthlimit: TFunc_THandle_Int;
  Fset_break_at_value: TProc_THandle_Double;
  Fget_break_at_value: TFunc_THandle_Double;
  Fset_negrange: TProc_THandle_Double;
  Fget_negrange: TFunc_THandle_Double;
  Fset_epsperturb: TProc_THandle_Double;
  Fget_epsperturb: TFunc_THandle_Double;
  Fset_epspivot: TProc_THandle_Double;
  Fget_epspivot: TFunc_THandle_Double;
  Fget_max_level: TFunc_THandle_Int;
  Fget_total_nodes: TFunc_THandle_COUNTER;
  Fget_total_iter: TFunc_THandle_COUNTER;
  Fget_objective: TFunc_THandle_Double;
  Fget_working_objective: TFunc_THandle_Double;
  Fget_var_primalresult: TFunc_THandle_Int_Double2;
  Fget_var_dualresult: TFunc_THandle_Int_Double2;
  Fget_variables: TFunc_THandle_PFloat_Bool;
  Fget_ptr_variables: TFunc_THandle_pPFloat_Bool;
  Fget_constraints: TFunc_THandle_PFloat_Bool;
  Fget_ptr_constraints: TFunc_THandle_pPFloat_Bool;
  Fget_sensitivity_rhs: TFunc_THandle_PFloat3_Bool;
  Fget_ptr_sensitivity_rhs: TFunc_THandle_pPFloat3_Bool;
  Fget_sensitivity_obj: TFunc_THandle_PFloat2_Bool;
  Fget_sensitivity_objex: TFunc_THandle_PFloat4_Bool;
  Fget_ptr_sensitivity_obj: TFunc_THandle_pPFloat2_Bool;
  Fget_ptr_sensitivity_objex: TFunc_THandle_pPFloat4_Bool;
  Fset_solutionlimit: TProc_THandle_Int;
  Fget_solutionlimit: TFunc_THandle_Int;
  Fget_solutioncount: TFunc_THandle_Int;
  Fget_Norig_rows: TFunc_THandle_Int;
  Fget_Nrows: TFunc_THandle_Int;
  Fget_Lrows: TFunc_THandle_Int;
  Fget_Norig_columns: TFunc_THandle_Int;
  Fget_Ncolumns: TFunc_THandle_Int;
  Fget_nameindex: TFunc_THandle_Str_Bool_Int;
{$IFDEF LPS55_UP}
  Fcopy_lp: TFunc_THandle_THandle;
  Fdualize_lp: TFunc_THandle_Bool;
  Fget_constr_value: TFunc_THandle_Int2_PFloat_PInt_Double;
  Fget_columnex: TFunc_THandle_Int_PFloat_PInt_Int;
  Fset_unbounded: TFunc_THandle_Int_Bool2;
  Fis_unbounded: TFunc_THandle_Int_Bool;
  Fget_basis: TFunc_THandle_PInt_Bool_Bool;
  Fset_basisvar: TFunc_THandle_Int2_Int;
  Fput_bb_nodefunc: TProc_THandle_lphandleint_intfunc_Pointer;
  Fput_bb_branchfunc: TProc_THandle_lphandleint_intfunc_Pointer;
  Fput_abortfunc: TProc_THandle_lphandle_intfunc_Pointer;
  Fput_logfunc: TProc_THandle_lphandlestr_func_Pointer;
  Fput_msgfunc: TProc_THandle_lphandleint_func_Pointer_Int;
  Fwrite_params: TFunc_THandle_Str2_Bool;
  Fread_params: TFunc_THandle_Str2_Bool;
  Freset_params: TProc_THandle;
  Fset_presolve: TProc_THandle_Int2;
  Fget_presolveloops: TFunc_THandle_Int;
  Fset_epslevel: TFunc_THandle_Int_Bool2;
  Fget_rowex: TFunc_THandle_Int_PFloat_PInt_Int;
  Fis_use_names: TFunc_THandle_Bool_Bool;
  Fset_use_names: TProc_THandle_Bool2;
  Fis_obj_in_basis: TFunc_THandle_Bool;
  Fset_obj_in_basis: TProc_THandle_Bool;
  Fget_accuracy: TFunc_THandle_Double;
  Fget_break_numeric_accuracy: TFunc_THandle_Double;
  Fset_break_numeric_accuracy: TProc_THandle_Double;
  Fset_pseudocosts: function(lp: THandle; clower: PFloatArray; cupper: PFloatArray; updatelimit: PIntArray): boolean; stdcall;
  Fget_pseudocosts: function(lp: THandle; clower: PFloatArray; cupper: PFloatArray; updatelimit: PIntArray): boolean; stdcall;
{$ELSE}
  Fset_free: TFunc_THandle_Int_Bool2;
  Fis_free: TFunc_THandle_Bool;
  Fget_basis: TProc_THandle_PInt_Bool;
  Fput_abortfunc: TProc_THandle_ctrlcfunc_Pointer;
  Fput_logfunc: TProc_THandle_logfunc_Pointer;
  Fput_msgfunc: TProc_THandle_msgfunc_Pointer_Int;
  Fset_presolve: TProc_THandle_Int;
{$ENDIF}

{$ENDIF}

procedure lp_solve_version(majorversion: Pinteger; minorversion: Pinteger; release: Pinteger; build: Pinteger); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'lp_solve_version';
{$ELSE}
begin
  if Assigned(Flp_solve_version) then
    Flp_solve_version(majorversion, minorversion, release, build);
end;
{$ENDIF}

function make_lp(rows: integer; columns: integer): THandle; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'make_lp';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fmake_lp) then
    Result := Fmake_lp(rows, columns);
end;
{$ENDIF}

function resize_lp(lp: THandle; rows: integer; columns: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'resize_lp';
{$ELSE}
begin
  Result := false;
  if Assigned(Fresize_lp) then
    Result := Fresize_lp(lp, rows, columns);
end;
{$ENDIF}

function get_statustext(lp: THandle; statuscode: integer): PAnsiChar; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_statustext';
{$ELSE}
begin
  Result := nil;
  if Assigned(Fget_statustext) then
    Result := Fget_statustext(lp, statuscode);
end;
{$ENDIF}

procedure delete_lp(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'delete_lp';
{$ELSE}
begin
  if Assigned(Fdelete_lp) then
    Fdelete_lp(lp);
end;
{$ENDIF}

procedure free_lp(var plp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'free_lp';
{$ELSE}
begin
  if Assigned(Ffree_lp) then
    Ffree_lp(plp);
end;
{$ENDIF}

function set_lp_name(lp: THandle; lpname: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_lp_name';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_lp_name) then
    Result := Fset_lp_name(lp, lpname);
end;
{$ENDIF}

function get_lp_name(lp: THandle): PAnsiChar; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_lp_name';
{$ELSE}
begin
  Result := nil;
  if Assigned(Fget_lp_name) then
    Result := Fget_lp_name(lp);
end;
{$ENDIF}

function has_BFP(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'has_BFP';
{$ELSE}
begin
  Result := false;
  if Assigned(Fhas_BFP) then
    Result := Fhas_BFP(lp);
end;
{$ENDIF}

function is_nativeBFP(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_nativeBFP';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_nativeBFP) then
    Result := Fis_nativeBFP(lp);
end;
{$ENDIF}

function set_BFP(lp: THandle; filename: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_BFP';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_BFP) then
    Result := Fset_BFP(lp, filename);
end;
{$ENDIF}

function read_XLI(xliname, modelname, dataname, options: PAnsiChar; verbose: integer): THandle; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'read_XLI';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fread_XLI) then
    Result := Fread_XLI(xliname, modelname, dataname, options, verbose);
end;
{$ENDIF}

function write_XLI(lp: THandle; filename, options: PAnsiChar; results: boolean): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'write_XLI';
{$ELSE}
begin
  Result := false;
  if Assigned(Fwrite_XLI) then
    Result := Fwrite_XLI(lp, filename, options, results);
end;
{$ENDIF}

function has_XLI(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'has_XLI';
{$ELSE}
begin
  Result := false;
  if Assigned(Fhas_XLI) then
    Result := Fhas_XLI(lp);
end;
{$ENDIF}

function is_nativeXLI(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_nativeXLI';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_nativeXLI) then
    Result := Fis_nativeXLI(lp);
end;
{$ENDIF}

function set_XLI(lp: THandle; filename: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_XLI';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_XLI) then
    Result := Fset_XLI(lp, filename);
end;
{$ENDIF}

function set_obj(lp: THandle; Column: integer; Value: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_obj';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_obj) then
    Result := Fset_obj(lp, Column, Value);
end;
{$ENDIF}

function set_obj_fn(lp: THandle; row: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_obj_fn';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_obj_fn) then
    Result := Fset_obj_fn(lp, row);
end;
{$ENDIF}

function set_obj_fnex(lp: THandle; count: integer; row: PFloatArray; colno: PIntArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_obj_fnex';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_obj_fnex) then
    Result := Fset_obj_fnex(lp, count, row, colno);
end;
{$ENDIF}

function str_set_obj_fn(lp: THandle; row_string: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'str_set_obj_fn';
{$ELSE}
begin
  Result := false;
  if Assigned(Fstr_set_obj_fn) then
    Result := Fstr_set_obj_fn(lp, row_string);
end;
{$ENDIF}

procedure set_sense(lp: THandle; maximize: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_sense';
{$ELSE}
begin
  if Assigned(Fset_sense) then
    Fset_sense(lp, maximize);
end;
{$ENDIF}

procedure set_maxim(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_maxim';
{$ELSE}
begin
  if Assigned(Fset_maxim) then
    Fset_maxim(lp);
end;
{$ENDIF}

procedure set_minim(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_minim';
{$ELSE}
begin
  if Assigned(Fset_minim) then
    Fset_minim(lp);
end;
{$ENDIF}

function is_maxim(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_maxim';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_maxim) then
    Result := Fis_maxim(lp);
end;
{$ENDIF}

function add_constraint(lp: THandle; row: PFloatArray; constr_type: integer; rh: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'add_constraint';
{$ELSE}
begin
  Result := false;
  if Assigned(Fadd_constraint) then
    Result := Fadd_constraint(lp, row, constr_type, rh);
end;
{$ENDIF}

function add_constraintex(lp: THandle; count: integer; row: PFloatArray; colno: PIntArray; constr_type: integer; rh: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'add_constraintex';
{$ELSE}
begin
  Result := false;
  if Assigned(Fadd_constraintex) then
    Result := Fadd_constraintex(lp, count, row, colno, constr_type, rh);
end;
{$ENDIF}

function set_add_rowmode(lp: THandle; turnon: boolean): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_add_rowmode';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_add_rowmode) then
    Result := Fset_add_rowmode(lp, turnon);
end;
{$ENDIF}

function is_add_rowmode(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_add_rowmode';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_add_rowmode) then
    Result := Fis_add_rowmode(lp);
end;
{$ENDIF}

function str_add_constraint(lp: THandle; row_string : PAnsiChar;constr_type: integer; rh: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'str_add_constraint';
{$ELSE}
begin
  Result := false;
  if Assigned(Fstr_add_constraint) then
    Result := Fstr_add_constraint(lp, row_string, constr_type, rh);
end;
{$ENDIF}

function get_row(lp: THandle; row_nr: integer; row: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_row';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_row) then
    Result := Fget_row(lp, row_nr, row);
end;
{$ENDIF}

function del_constraint(lp: THandle; del_row: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'del_constraint';
{$ELSE}
begin
  Result := false;
  if Assigned(Fdel_constraint) then
    Result := Fdel_constraint(lp, del_row);
end;
{$ENDIF}

function add_lag_con(lp: THandle; row: PFloatArray; con_type: integer; rhs: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'add_lag_con';
{$ELSE}
begin
  Result := false;
  if Assigned(Fadd_lag_con) then
    Result := Fadd_lag_con(lp, row, con_type, rhs);
end;
{$ENDIF}

function str_add_lag_con(lp: THandle; row_string: PAnsiChar; con_type: integer; rhs: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'str_add_lag_con';
{$ELSE}
begin
  Result := false;
  if Assigned(Fstr_add_lag_con) then
    Result := Fstr_add_lag_con(lp, row_string, con_type, rhs);
end;
{$ENDIF}

procedure set_lag_trace(lp: THandle; lag_trace: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_lag_trace';
{$ELSE}
begin
  if Assigned(Fset_lag_trace) then
    Fset_lag_trace(lp, lag_trace);
end;
{$ENDIF}

function is_lag_trace(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_lag_trace';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_lag_trace) then
    Result := Fis_lag_trace(lp);
end;
{$ENDIF}

function set_constr_type(lp: THandle; row: integer; con_type: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_constr_type';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_constr_type) then
    Result := Fset_constr_type(lp, row, con_type);
end;
{$ENDIF}

function get_constr_type(lp: THandle; row: integer): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_constr_type';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_constr_type) then
    Result := Fget_constr_type(lp, row);
end;
{$ENDIF}

function is_constr_type(lp: THandle; row: integer; mask: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_constr_type';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_constr_type) then
    Result := Fis_constr_type(lp, row, mask);
end;
{$ENDIF}

function set_rh(lp: THandle; row: integer; value: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_rh';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_rh) then
    Result := Fset_rh(lp, row, value);
end;
{$ENDIF}

function get_rh(lp: THandle; row: integer): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_rh';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_rh) then
    Result := Fget_rh(lp, row);
end;
{$ENDIF}

function set_rh_range(lp: THandle; row: integer; deltavalue: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_rh_range';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_rh_range) then
    Result := Fset_rh_range(lp, row, deltavalue);
end;
{$ENDIF}

function get_rh_range(lp: THandle; row: integer): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_rh_range';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_rh_range) then
    Result := Fget_rh_range(lp, row);
end;
{$ENDIF}

procedure set_rh_vec(lp: THandle; rh: PFloatArray); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_rh_vec';
{$ELSE}
begin
  if Assigned(Fset_rh_vec) then
    Fset_rh_vec(lp, rh);
end;
{$ENDIF}

function str_set_rh_vec(lp: THandle; rh_string: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'str_set_rh_vec';
{$ELSE}
begin
  Result := false;
  if Assigned(Fstr_set_rh_vec) then
    Result := Fstr_set_rh_vec(lp, rh_string);
end;
{$ENDIF}

function add_column(lp: THandle; column: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'add_column';
{$ELSE}
begin
  Result := false;
  if Assigned(Fadd_column) then
    Result := Fadd_column(lp, column);
end;
{$ENDIF}

function add_columnex(lp: THandle; count: integer; column: PFloatArray; rowno: PIntArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'add_columnex';
{$ELSE}
begin
  Result := false;
  if Assigned(Fadd_columnex) then
    Result := Fadd_columnex(lp, count, column, rowno);
end;
{$ENDIF}

function str_add_column(lp: THandle; col_string: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'str_add_column';
{$ELSE}
begin
  Result := false;
  if Assigned(Fstr_add_column) then
    Result := Fstr_add_column(lp, col_string);
end;
{$ENDIF}

function column_in_lp(lp: THandle; column: PFloatArray): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'column_in_lp';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fcolumn_in_lp) then
    Result := Fcolumn_in_lp(lp, column);
end;
{$ENDIF}

function get_column(lp: THandle; col_nr: integer; column: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_column';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_column) then
    Result := Fget_column(lp, col_nr, column);
end;
{$ENDIF}

function del_column(lp: THandle; column: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'del_column';
{$ELSE}
begin
  Result := false;
  if Assigned(Fdel_column) then
    Result := Fdel_column(lp, column);
end;
{$ENDIF}

function set_mat(lp: THandle; row: integer; column: integer; value: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_mat';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_mat) then
    Result := Fset_mat(lp, row, column, value);
end;
{$ENDIF}

function get_mat(lp: THandle; row: integer; column: integer): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_mat';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_mat) then
    Result := Fget_mat(lp, row, column);
end;
{$ENDIF}

function get_mat_byindex(lp: THandle; matindex: Integer; isrow, adjustsign: boolean): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_mat_byindex';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_mat_byindex) then
    Result := Fget_mat_byindex(lp, matindex, isrow, adjustsign);
end;
{$ENDIF}

procedure set_bounds_tighter(lp: THandle; tighten: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_bounds_tighter';
{$ELSE}
begin
  if Assigned(Fset_bounds_tighter) then
    Fset_bounds_tighter(lp, tighten);
end;
{$ENDIF}

function get_bounds_tighter(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_bounds_tighter';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_bounds_tighter) then
    Result := Fget_bounds_tighter(lp);
end;
{$ENDIF}

function set_upbo(lp: THandle; column: integer; value: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_upbo';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_upbo) then
    Result := Fset_upbo(lp, column, value);
end;
{$ENDIF}

function get_upbo(lp: THandle; column: integer): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_upbo';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_upbo) then
    Result := Fget_upbo(lp, column);
end;
{$ENDIF}

function set_lowbo(lp: THandle; column: integer; value: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_lowbo';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_lowbo) then
    Result := Fset_lowbo(lp, column, value);
end;
{$ENDIF}

function get_lowbo(lp: THandle; column: integer): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_lowbo';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_lowbo) then
    Result := Fget_lowbo(lp, column);
end;
{$ENDIF}

function set_bounds(lp: THandle; column: integer; lower: double; upper: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_bounds';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_bounds) then
    Result := Fset_bounds(lp, column, lower, upper);
end;
{$ENDIF}

function set_int(lp: THandle; column: integer; must_be_int: boolean): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_int';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_int) then
    Result := Fset_int(lp, column, must_be_int);
end;
{$ENDIF}

function is_int(lp: THandle; column: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_int';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_int) then
    Result := Fis_int(lp, column);
end;
{$ENDIF}

function set_binary(lp: THandle; column: integer; must_be_bin: boolean): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_binary';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_binary) then
    Result := Fset_binary(lp, column, must_be_bin);
end;
{$ENDIF}

function is_binary(lp: THandle; column: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_binary';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_binary) then
    Result := Fis_binary(lp, column);
end;
{$ENDIF}

function set_semicont(lp: THandle; column: integer; must_be_sc: boolean): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_semicont';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_semicont) then
    Result := Fset_semicont(lp, column, must_be_sc);
end;
{$ENDIF}

function is_semicont(lp: THandle; column: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_semicont';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_semicont) then
    Result := Fis_semicont(lp, column);
end;
{$ENDIF}

function is_negative(lp: THandle; column: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_negative';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_negative) then
    Result := Fis_negative(lp, column);
end;
{$ENDIF}

function set_var_weights(lp: THandle; weights: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_var_weights';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_var_weights) then
    Result := Fset_var_weights(lp, weights);
end;
{$ENDIF}

function get_var_priority(lp: THandle; column: integer): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_var_priority';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_var_priority) then
    Result := Fget_var_priority(lp, column);
end;
{$ENDIF}

function add_SOS(lp: THandle; name: PAnsiChar; sostype: integer; priority: integer; count: integer; sosvars: PIntArray; weights: PFloatArray): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'add_SOS';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fadd_SOS) then
    Result := Fadd_SOS(lp, name, sostype, priority, count, sosvars, weights);
end;
{$ENDIF}

function is_SOS_var(lp: THandle; column: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_SOS_var';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_SOS_var) then
    Result := Fis_SOS_var(lp, column);
end;
{$ENDIF}

function set_row_name(lp: THandle; row: integer; new_name: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_row_name';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_row_name) then
    Result := Fset_row_name(lp, row, new_name);
end;
{$ENDIF}

function get_row_name(lp: THandle; row: integer): PAnsiChar; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_row_name';
{$ELSE}
begin
  Result := nil;
  if Assigned(Fget_row_name) then
    Result := Fget_row_name(lp, row);
end;
{$ENDIF}

function get_origrow_name(lp: THandle; row: integer): PAnsiChar; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_origrow_name';
{$ELSE}
begin
  Result := nil;
  if Assigned(Fget_origrow_name) then
    Result := Fget_origrow_name(lp, row);
end;
{$ENDIF}

function set_col_name(lp: THandle; column: integer; new_name: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_col_name';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_col_name) then
    Result := Fset_col_name(lp, column, new_name);
end;
{$ENDIF}

function get_col_name(lp: THandle; column: integer): PAnsiChar; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_col_name';
{$ELSE}
begin
  Result := nil;
  if Assigned(Fget_col_name) then
    Result := Fget_col_name(lp, column);
end;
{$ENDIF}

function get_origcol_name(lp: THandle; column: integer): PAnsiChar; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_origcol_name';
{$ELSE}
begin
  Result := nil;
  if Assigned(Fget_origcol_name) then
    Result := Fget_origcol_name(lp, column);
end;
{$ENDIF}

procedure unscale(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'unscale';
{$ELSE}
begin
  if Assigned(Funscale) then
    Funscale(lp);
end;
{$ENDIF}

procedure set_preferdual(lp: THandle; dodual: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_preferdual';
{$ELSE}
begin
  if Assigned(Fset_preferdual) then
    Fset_preferdual(lp, dodual);
end;
{$ENDIF}

procedure set_simplextype(lp: THandle; simplextype: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_simplextype';
{$ELSE}
begin
  if Assigned(Fset_simplextype) then
    Fset_simplextype(lp, simplextype);
end;
{$ENDIF}

function get_simplextype(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_simplextype';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_simplextype) then
    Result := Fget_simplextype(lp);
end;
{$ENDIF}

procedure default_basis(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'default_basis';
{$ELSE}
begin
  if Assigned(Fdefault_basis) then
    Fdefault_basis(lp);
end;
{$ENDIF}

procedure set_basiscrash(lp: THandle; mode: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_basiscrash';
{$ELSE}
begin
  if Assigned(Fset_basiscrash) then
    Fset_basiscrash(lp, mode);
end;
{$ENDIF}

function get_basiscrash(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_basiscrash';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_basiscrash) then
    Result := Fget_basiscrash(lp);
end;
{$ENDIF}

function set_basis(lp: THandle; bascolumn: PIntArray; nonbasic: boolean): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_basis';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_basis) then
    Result := Fset_basis(lp, bascolumn, nonbasic);
end;
{$ENDIF}

function is_feasible(lp: THandle; values: PFloatArray; threshold: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_feasible';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_feasible) then
    Result := Fis_feasible(lp, values, threshold);
end;
{$ENDIF}

function solve(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'solve';
{$ELSE}
begin
  Result := UNKNOWNERROR;
  if Assigned(Fsolve) then
    Result := Fsolve(lp);
end;
{$ENDIF}

function time_elapsed(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'time_elapsed';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Ftime_elapsed) then
    Result := Ftime_elapsed(lp);
end;
{$ENDIF}

function get_primal_solution(lp: THandle; pv: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_primal_solution';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_primal_solution) then
    Result := Fget_primal_solution(lp, pv);
end;
{$ENDIF}

function get_ptr_primal_solution(lp: THandle; var pv: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_ptr_primal_solution';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_ptr_primal_solution) then
    Result := Fget_ptr_primal_solution(lp, pv);
end;
{$ENDIF}

function get_dual_solution(lp: THandle; rc: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_dual_solution';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_dual_solution) then
    Result := Fget_dual_solution(lp, rc);
end;
{$ENDIF}

function get_ptr_dual_solution(lp: THandle; var rc: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_ptr_dual_solution';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_ptr_dual_solution) then
    Result := Fget_ptr_dual_solution(lp, rc);
end;
{$ENDIF}

function get_lambda(lp: THandle; lambda: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_lambda';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_lambda) then
    Result := Fget_lambda(lp, lambda);
end;
{$ENDIF}

function get_ptr_lambda(lp: THandle; var lambda: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_ptr_lambda';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_ptr_lambda) then
    Result := Fget_ptr_lambda(lp, lambda);
end;
{$ENDIF}

procedure reset_basis(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'reset_basis';
{$ELSE}
begin
  if Assigned(Freset_basis) then
    Freset_basis(lp);
end;
{$ENDIF}

function read_MPS(filename: PAnsiChar; options: integer): THandle; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'read_MPS';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fread_MPS_file) then
    Result := Fread_MPS_file(filename, options);
end;
{$ENDIF}

function read_mps(stream: PInteger; options: integer): THandle; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'read_mps';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fread_mps_stream) then
    Result := Fread_mps_stream(stream, options);
end;
{$ENDIF}

function read_freeMPS(filename: PAnsiChar; options: Integer): THandle; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'read_freeMPS';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fread_freeMPS_file) then
    Result := Fread_freeMPS_file(filename, options);
end;
{$ENDIF}

function read_freemps(filename: Pinteger; options: Integer): THandle; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'read_freemps';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fread_freemps_stream) then
    Result := Fread_freemps_stream(filename, options);
end;
{$ENDIF}

function write_freemps(lp: THandle; filename: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'write_freemps';
{$ELSE}
begin
  Result := false;
  if Assigned(Fwrite_freemps_file) then
    Result := Fwrite_freemps_file(lp, filename);
end;
{$ENDIF}

function write_freeMPS(lp: THandle; output: PInteger): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'write_freeMPS';
{$ELSE}
begin
  Result := false;
  if Assigned(Fwrite_freeMPS_stream) then
    Result := Fwrite_freeMPS_stream(lp, output);
end;
{$ENDIF}

function guess_basis(lp: THandle; guessvector: PFloatArray; basisvector: PIntArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'guess_basis';
{$ELSE}
begin
  Result := false;
  if Assigned(Fguess_basis) then
    Result := Fguess_basis(lp, guessvector, basisvector);
end;
{$ENDIF}

function read_basis(lp: THandle; filename, info: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'read_basis';
{$ELSE}
begin
  Result := false;
  if Assigned(Fread_basis) then
    Result := Fread_basis(lp, filename, info);
end;
{$ENDIF}

function write_basis(lp: THandle; filename: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'write_basis';
{$ELSE}
begin
  Result := false;
  if Assigned(Fwrite_basis) then
    Result := Fwrite_basis(lp, filename);
end;
{$ENDIF}

function write_mps(lp: THandle; filename: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'write_mps';
{$ELSE}
begin
  Result := false;
  if Assigned(Fwrite_mps_file) then
    Result := Fwrite_mps_file(lp, filename);
end;
{$ENDIF}

function write_MPS(lp: THandle; output: PInteger): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'write_MPS';
{$ELSE}
begin
  Result := false;
  if Assigned(Fwrite_MPS_stream) then
    Result := Fwrite_MPS_stream(lp, output);
end;
{$ENDIF}

function write_lp(lp: THandle; filename: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'write_lp';
{$ELSE}
begin
  Result := false;
  if Assigned(Fwrite_lp_file) then
    Result := Fwrite_lp_file(lp, filename);
end;
{$ENDIF}

function write_LP(lp: THandle; filename: PInteger): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'write_LP';
{$ELSE}
begin
  Result := false;
  if Assigned(Fwrite_LP_stream) then
    Result := Fwrite_LP_stream(lp, filename);
end;
{$ENDIF}

function read_lp(filename: PInteger; verbose: integer; lp_name: PAnsiChar): THandle; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'read_lp';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fread_LP_stream) then
    Result := Fread_LP_stream(filename, verbose, lp_name);
end;
{$ENDIF}

function read_LP(filename: PAnsiChar; verbose: integer; lp_name: PAnsiChar): THandle; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'read_LP';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fread_lp_file) then
    Result := Fread_lp_file(filename, verbose, lp_name);
end;
{$ENDIF}

procedure print_lp(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'print_lp';
{$ELSE}
begin
  if Assigned(Fprint_lp) then
    Fprint_lp(lp);
end;
{$ENDIF}

procedure print_tableau(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'print_tableau';
{$ELSE}
begin
  if Assigned(Fprint_tableau) then
    Fprint_tableau(lp);
end;
{$ENDIF}

procedure print_objective(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'print_objective';
{$ELSE}
begin
  if Assigned(Fprint_objective) then
    Fprint_objective(lp);
end;
{$ENDIF}

procedure print_solution(lp: THandle; columns: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'print_solution';
{$ELSE}
begin
  if Assigned(Fprint_solution) then
    Fprint_solution(lp, columns);
end;
{$ENDIF}

procedure print_constraints(lp: THandle; columns: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'print_constraints';
{$ELSE}
begin
  if Assigned(Fprint_constraints) then
    Fprint_constraints(lp, columns);
end;
{$ENDIF}

procedure print_duals(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'print_duals';
{$ELSE}
begin
  if Assigned(Fprint_duals) then
    Fprint_duals(lp);
end;
{$ENDIF}

procedure print_scales(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'print_scales';
{$ELSE}
begin
  if Assigned(Fprint_scales) then
    Fprint_scales(lp);
end;
{$ENDIF}

procedure print_str(lp: THandle; str: PAnsiChar); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'print_str';
{$ELSE}
begin
  if Assigned(Fprint_str) then
    Fprint_str(lp, str);
end;
{$ENDIF}

procedure set_outputstream(lp: THandle; stream: Pointer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_outputstream';
{$ELSE}
begin
  if Assigned(Fset_outputstream) then
    Fset_outputstream(lp, stream);
end;
{$ENDIF}

function set_outputfile(lp: THandle; filename: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_outputfile';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_outputfile) then
    Result := Fset_outputfile(lp, filename);
end;
{$ENDIF}

procedure set_verbose(lp: THandle; verbose: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_verbose';
{$ELSE}
begin
  if Assigned(Fset_verbose) then
    Fset_verbose(lp, verbose);
end;
{$ENDIF}

function get_verbose(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_verbose';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_verbose) then
    Result := Fget_verbose(lp);
end;
{$ENDIF}

procedure set_timeout(lp: THandle; sectimeout: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_timeout';
{$ELSE}
begin
  if Assigned(Fset_timeout) then
    Fset_timeout(lp, sectimeout);
end;
{$ENDIF}

function get_timeout(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_timeout';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_timeout) then
    Result := Fget_timeout(lp);
end;
{$ENDIF}

procedure set_print_sol(lp: THandle; print_sol: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_print_sol';
{$ELSE}
begin
  if Assigned(Fset_print_sol) then
    Fset_print_sol(lp, print_sol);
end;
{$ENDIF}

function get_print_sol(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_print_sol';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_print_sol) then
    Result := Fget_print_sol(lp);
end;
{$ENDIF}

procedure set_debug(lp: THandle; debug: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_debug';
{$ELSE}
begin
  if Assigned(Fset_debug) then
    Fset_debug(lp, debug);
end;
{$ENDIF}

function is_debug(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_debug';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_debug) then
    Result := Fis_debug(lp);
end;
{$ENDIF}

procedure set_trace(lp: THandle; trace: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_trace';
{$ELSE}
begin
  if Assigned(Fset_trace) then
    Fset_trace(lp, trace);
end;
{$ENDIF}

function is_trace(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_trace';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_trace) then
    Result := Fis_trace(lp);
end;
{$ENDIF}

function print_debugdump(lp: THandle; filename: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'print_debugdump';
{$ELSE}
begin
  Result := false;
  if Assigned(Fprint_debugdump) then
    Result := Fprint_debugdump(lp, filename);
end;
{$ENDIF}

procedure set_anti_degen(lp: THandle; anti_degen: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_anti_degen';
{$ELSE}
begin
  if Assigned(Fset_anti_degen) then
    Fset_anti_degen(lp, anti_degen);
end;
{$ENDIF}

function get_anti_degen(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_anti_degen';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_anti_degen) then
    Result := Fget_anti_degen(lp);
end;
{$ENDIF}

function is_anti_degen(lp: THandle; testmask: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_anti_degen';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_anti_degen) then
    Result := Fis_anti_degen(lp, testmask);
end;
{$ENDIF}

function get_presolve(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_presolve';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_presolve) then
    Result := Fget_presolve(lp);
end;
{$ENDIF}

function is_presolve(lp: THandle; testmask: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_presolve';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_presolve) then
    Result := Fis_presolve(lp, testmask);
end;
{$ENDIF}

function get_orig_index(lp: THandle; lp_index: integer): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_orig_index';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_orig_index) then
    Result := Fget_orig_index(lp, lp_index);
end;
{$ENDIF}

function get_lp_index(lp: THandle; orig_index: integer): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_lp_index';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_lp_index) then
    Result := Fget_lp_index(lp, orig_index);
end;
{$ENDIF}

procedure set_maxpivot(lp: THandle; max_num_inv: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_maxpivot';
{$ELSE}
begin
  if Assigned(Fset_maxpivot) then
    Fset_maxpivot(lp, max_num_inv);
end;
{$ENDIF}

function get_maxpivot(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_maxpivot';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_maxpivot) then
    Result := Fget_maxpivot(lp);
end;
{$ENDIF}

procedure set_obj_bound(lp: THandle; obj_bound: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_obj_bound';
{$ELSE}
begin
  if Assigned(Fset_obj_bound) then
    Fset_obj_bound(lp, obj_bound);
end;
{$ENDIF}

function get_obj_bound(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_obj_bound';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_obj_bound) then
    Result := Fget_obj_bound(lp);
end;
{$ENDIF}

procedure set_mip_gap(lp: THandle; absolute: boolean; mip_gap: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_mip_gap';
{$ELSE}
begin
  if Assigned(Fset_mip_gap) then
    Fset_mip_gap(lp, absolute, mip_gap);
end;
{$ENDIF}

function get_mip_gap(lp: THandle; absolute: boolean): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_mip_gap';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_mip_gap) then
    Result := Fget_mip_gap(lp, absolute);
end;
{$ENDIF}

procedure set_bb_rule(lp: THandle; bb_rule: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_bb_rule';
{$ELSE}
begin
  if Assigned(Fset_bb_rule) then
    Fset_bb_rule(lp, bb_rule);
end;
{$ENDIF}

function get_bb_rule(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_bb_rule';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_bb_rule) then
    Result := Fget_bb_rule(lp);
end;
{$ENDIF}

function set_var_branch(lp: THandle; column: integer; branch_mode: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_var_branch';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_var_branch) then
    Result := Fset_var_branch(lp, column, branch_mode);
end;
{$ENDIF}

function get_var_branch(lp: THandle; column: integer): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_var_branch';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_var_branch) then
    Result := Fget_var_branch(lp, column);
end;
{$ENDIF}

procedure set_infinite(lp: THandle; infinite: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_infinite';
{$ELSE}
begin
  if Assigned(Fset_infinite) then
    Fset_infinite(lp, infinite);
end;
{$ENDIF}

function get_infinite(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_infinite';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_infinite) then
    Result := Fget_infinite(lp);
end;
{$ENDIF}

procedure set_epsint(lp: THandle; epsilon: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_epsint';
{$ELSE}
begin
  if Assigned(Fset_epsint) then
    Fset_epsint(lp, epsilon);
end;
{$ENDIF}

function get_epsint(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_epsint';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_epsint) then
    Result := Fget_epsint(lp);
end;
{$ENDIF}

procedure set_epsb(lp: THandle; epsb: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_epsb';
{$ELSE}
begin
  if Assigned(Fset_epsb) then
    Fset_epsb(lp, epsb);
end;
{$ENDIF}

function get_epsb(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_epsb';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_epsb) then
    Result := Fget_epsb(lp);
end;
{$ENDIF}

procedure set_epsd(lp: THandle; epsd: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_epsd';
{$ELSE}
begin
  if Assigned(Fset_epsd) then
    Fset_epsd(lp, epsd);
end;
{$ENDIF}

function get_epsd(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_epsd';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_epsd) then
    Result := Fget_epsd(lp);
end;
{$ENDIF}

procedure set_epsel(lp: THandle; epsel: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_epsel';
{$ELSE}
begin
  if Assigned(Fset_epsel) then
    Fset_epsel(lp, epsel);
end;
{$ENDIF}

function get_epsel(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_epsel';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_epsel) then
    Result := Fget_epsel(lp);
end;
{$ENDIF}

procedure set_scaling(lp: THandle; scalemode: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_scaling';
{$ELSE}
begin
  if Assigned(Fset_scaling) then
    Fset_scaling(lp, scalemode);
end;
{$ENDIF}

function get_scaling(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_scaling';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_scaling) then
    Result := Fget_scaling(lp);
end;
{$ENDIF}

function is_scalemode(lp: THandle; testmask: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_scalemode';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_scalemode) then
    Result := Fis_scalemode(lp, testmask);
end;
{$ENDIF}

function is_scaletype(lp: THandle; scaletype: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_scaletype';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_scaletype) then
    Result := Fis_scaletype(lp, scaletype);
end;
{$ENDIF}

function is_integerscaling(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_integerscaling';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_integerscaling) then
    Result := Fis_integerscaling(lp);
end;
{$ENDIF}

procedure set_scalelimit(lp: THandle; scalelimit: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_scalelimit';
{$ELSE}
begin
  if Assigned(Fset_scalelimit) then
    Fset_scalelimit(lp, scalelimit);
end;
{$ENDIF}

function get_scalelimit(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_scalelimit';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_scalelimit) then
    Result := Fget_scalelimit(lp);
end;
{$ENDIF}

procedure set_improve(lp: THandle; improve: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_improve';
{$ELSE}
begin
  if Assigned(Fset_improve) then
    Fset_improve(lp, improve);
end;
{$ENDIF}

function get_improve(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_improve';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_improve) then
    Result := Fget_improve(lp);
end;
{$ENDIF}

procedure set_pivoting(lp: THandle; piv_rule: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_pivoting';
{$ELSE}
begin
  if Assigned(Fset_pivoting) then
    Fset_pivoting(lp, piv_rule);
end;
{$ENDIF}

function get_pivoting(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_pivoting';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_pivoting) then
    Result := Fget_pivoting(lp);
end;
{$ENDIF}

function is_piv_mode(lp: THandle; testmask: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_piv_mode';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_piv_mode) then
    Result := Fis_piv_mode(lp, testmask);
end;
{$ENDIF}

function is_piv_rule(lp: THandle; rule: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_piv_rule';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_piv_rule) then
    Result := Fis_piv_rule(lp, rule);
end;
{$ENDIF}

procedure set_break_at_first(lp: THandle; break_at_first: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_break_at_first';
{$ELSE}
begin
  if Assigned(Fset_break_at_first) then
    Fset_break_at_first(lp, break_at_first);
end;
{$ENDIF}

function is_break_at_first(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_break_at_first';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_break_at_first) then
    Result := Fis_break_at_first(lp);
end;
{$ENDIF}

procedure set_bb_floorfirst(lp: THandle; bb_floorfirst: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_bb_floorfirst';
{$ELSE}
begin
  if Assigned(Fset_bb_floorfirst) then
    Fset_bb_floorfirst(lp, bb_floorfirst);
end;
{$ENDIF}

function get_bb_floorfirst(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_bb_floorfirst';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_bb_floorfirst) then
    Result := Fget_bb_floorfirst(lp);
end;
{$ENDIF}

procedure set_bb_depthlimit(lp: THandle; bb_maxlevel: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_bb_depthlimit';
{$ELSE}
begin
  if Assigned(Fset_bb_depthlimit) then
    Fset_bb_depthlimit(lp, bb_maxlevel);
end;
{$ENDIF}

function get_bb_depthlimit(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_bb_depthlimit';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_bb_depthlimit) then
    Result := Fget_bb_depthlimit(lp);
end;
{$ENDIF}

procedure set_break_at_value(lp: THandle; break_at_value: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_break_at_value';
{$ELSE}
begin
  if Assigned(Fset_break_at_value) then
    Fset_break_at_value(lp, break_at_value);
end;
{$ENDIF}

function get_break_at_value(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_break_at_value';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_break_at_value) then
    Result := Fget_break_at_value(lp);
end;
{$ENDIF}

procedure set_negrange(lp: THandle; negrange: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_negrange';
{$ELSE}
begin
  if Assigned(Fset_negrange) then
    Fset_negrange(lp, negrange);
end;
{$ENDIF}

function get_negrange(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_negrange';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_negrange) then
    Result := Fget_negrange(lp);
end;
{$ENDIF}

procedure set_epsperturb(lp: THandle; epsperturb: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_epsperturb';
{$ELSE}
begin
  if Assigned(Fset_epsperturb) then
    Fset_epsperturb(lp, epsperturb);
end;
{$ENDIF}

function get_epsperturb(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_epsperturb';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_epsperturb) then
    Result := Fget_epsperturb(lp);
end;
{$ENDIF}

procedure set_epspivot(lp: THandle; epspivot: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_epspivot';
{$ELSE}
begin
  if Assigned(Fset_epspivot) then
    Fset_epspivot(lp, epspivot);
end;
{$ENDIF}

function get_epspivot(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_epspivot';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_epspivot) then
    Result := Fget_epspivot(lp);
end;
{$ENDIF}

function get_max_level(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_max_level';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_max_level) then
    Result := Fget_max_level(lp);
end;
{$ENDIF}

function get_total_nodes(lp: THandle): COUNTER; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_total_nodes';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_total_nodes) then
    Result := Fget_total_nodes(lp);
end;
{$ENDIF}

function get_total_iter(lp: THandle): COUNTER; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_total_iter';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_total_iter) then
    Result := Fget_total_iter(lp);
end;
{$ENDIF}

function get_objective(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_objective';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_objective) then
    Result := Fget_objective(lp);
end;
{$ENDIF}

function get_working_objective(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_working_objective';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_working_objective) then
    Result := Fget_working_objective(lp);
end;
{$ENDIF}

function get_var_primalresult(lp: THandle; index: integer): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_var_primalresult';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_var_primalresult) then
    Result := Fget_var_primalresult(lp, index);
end;
{$ENDIF}

function get_var_dualresult(lp: THandle; index: integer): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_var_dualresult';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_var_dualresult) then
    Result := Fget_var_dualresult(lp, index);
end;
{$ENDIF}

function get_variables(lp: THandle; var_: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_variables';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_variables) then
    Result := Fget_variables(lp, var_);
end;
{$ENDIF}

function get_ptr_variables(lp: THandle; var var_: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_ptr_variables';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_ptr_variables) then
    Result := Fget_ptr_variables(lp, var_);
end;
{$ENDIF}

function get_constraints(lp: THandle; constr: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_constraints';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_constraints) then
    Result := Fget_constraints(lp, constr);
end;
{$ENDIF}

function get_ptr_constraints(lp: THandle; var constr: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_ptr_constraints';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_ptr_constraints) then
    Result := Fget_ptr_constraints(lp, constr);
end;
{$ENDIF}

function get_sensitivity_rhs(lp: THandle; duals, dualsfrom, dualstill: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_sensitivity_rhs';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_sensitivity_rhs) then
    Result := Fget_sensitivity_rhs(lp, duals, dualsfrom, dualstill);
end;
{$ENDIF}

function get_ptr_sensitivity_rhs(lp: THandle; var duals, dualsfrom, dualstill: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_ptr_sensitivity_rhs';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_ptr_sensitivity_rhs) then
    Result := Fget_ptr_sensitivity_rhs(lp, duals, dualsfrom, dualstill);
end;
{$ENDIF}

function get_sensitivity_obj(lp: THandle; objfrom, objtill: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_sensitivity_obj';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_sensitivity_obj) then
    Result := Fget_sensitivity_obj(lp, objfrom, objtill);
end;
{$ENDIF}

function get_ptr_sensitivity_obj(lp: THandle; var objfrom, objtill: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_ptr_sensitivity_obj';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_ptr_sensitivity_obj) then
    Result := Fget_ptr_sensitivity_obj(lp, objfrom, objtill);
end;
{$ENDIF}

procedure set_solutionlimit(lp: THandle; limit: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_solutionlimit';
{$ELSE}
begin
  if Assigned(Fset_solutionlimit) then
    Fset_solutionlimit(lp, limit);
end;
{$ENDIF}

function get_solutionlimit(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_solutionlimit';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_solutionlimit) then
    Result := Fget_solutionlimit(lp);
end;
{$ENDIF}

function get_solutioncount(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_solutioncount';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_solutioncount) then
    Result := Fget_solutioncount(lp);
end;
{$ENDIF}

function get_Norig_rows(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_Norig_rows';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_Norig_rows) then
    Result := Fget_Norig_rows(lp);
end;
{$ENDIF}

function get_Nrows(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_Nrows';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_Nrows) then
    Result := Fget_Nrows(lp);
end;
{$ENDIF}

function get_Lrows(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_Lrows';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_Lrows) then
    Result := Fget_Lrows(lp);
end;
{$ENDIF}

function get_Norig_columns(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_Norig_columns';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_Norig_columns) then
    Result := Fget_Norig_columns(lp);
end;
{$ENDIF}

function get_Ncolumns(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_Ncolumns';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_Ncolumns) then
    Result := Fget_Ncolumns(lp);
end;
{$ENDIF}

function get_nonzeros(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_nonzeros';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_nonzeros) then
    Result := Fget_nonzeros(lp);
end;
{$ENDIF}

function get_status(lp: THandle): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_status';
{$ELSE}
begin
  Result := NOTRUN;
  if Assigned(Fget_status) then
    Result := Fget_status(lp);
end;
{$ENDIF}

function is_infinite(lp: THandle; value: double): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_infinite';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_infinite) then
    Result := Fis_infinite(lp, value);
end;
{$ENDIF}

function set_column(lp: THandle; col_no: Integer; column: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_column';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_column) then
    Result := Fset_column(lp, col_no, column);
end;
{$ENDIF}

function set_columnex(lp: THandle; col_no, count: Integer; column: PFloatArray; rowno: PIntArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_columnex';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_columnex) then
    Result := Fset_columnex(lp, col_no, count, column, rowno);
end;
{$ENDIF}

function set_row(lp: THandle; row_no: Integer; row: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_row';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_row) then
    Result := Fset_row(lp, row_no, row);
end;
{$ENDIF}

function set_rowex(lp: THandle; row_no, count: Integer; row: PFloatArray; colno: PIntArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_rowex';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_rowex) then
    Result := Fset_rowex(lp, row_no, count, row, colno);
end;
{$ENDIF}

function get_ptr_sensitivity_objex(lp: THandle; var objfrom, objtill, objfromvalue, objtillvalue: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_ptr_sensitivity_objex';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_ptr_sensitivity_objex) then
    Result := Fget_ptr_sensitivity_objex(lp, objfrom, objtill, objfromvalue, objtillvalue);
end;
{$ENDIF}

function get_sensitivity_objex(lp: THandle; objfrom, objtill, objfromvalue, objtillvalue: PFloatArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_sensitivity_objex';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_sensitivity_objex) then
    Result := Fget_sensitivity_objex(lp, objfrom, objtill, objfromvalue, objtillvalue);
end;
{$ENDIF}

function get_nameindex(lp: THandle; varname: PAnsiChar; isrow: boolean): Integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_nameindex';
{$ELSE}
begin
  Result := -1;
  if Assigned(Fget_nameindex) then
    Result := Fget_nameindex(lp, varname, isrow);
end;
{$ENDIF}

function set_partialprice(lp: THandle; blockcount: Integer; blockstart: PIntArray; isrow: boolean): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_partialprice';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_partialprice) then
    Result := Fset_partialprice(lp, blockcount, blockstart, isrow);
end;
{$ENDIF}

procedure get_partialprice(lp: THandle; blockcount: PIntArray; blockstart: PIntArray; isrow: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_partialprice';
{$ELSE}
begin
  if Assigned(Fget_partialprice) then
    Fget_partialprice(lp, blockcount, blockstart, isrow);
end;
{$ENDIF}

function set_multiprice(lp: THandle; multiblockdiv: Integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_multiprice';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_multiprice) then
    Result := Fset_multiprice(lp, multiblockdiv);
end;
{$ENDIF}

function get_multiprice(lp: THandle; getabssize: boolean): Integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_multiprice';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_multiprice) then
    Result := Fget_multiprice(lp, getabssize);
end;
{$ENDIF}

{$IFDEF LPS55_UP}
function copy_lp(lp: THandle): THandle; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'copy_lp';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fcopy_lp) then
    Result := Fcopy_lp(lp);
end;
{$ENDIF}

function dualize_lp(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'dualize_lp';
{$ELSE}
begin
  Result := false;
  if Assigned(Fdualize_lp) then
    Result := Fdualize_lp(lp);
end;
{$ENDIF}

function get_columnex(lp: THandle; colnr: Integer; column: PFloatArray; nzrow: PIntArray): Integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_columnex';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_columnex) then
    Result := Fget_columnex(lp, colnr, column, nzrow);
end;
{$ENDIF}

function get_constr_value(lp: THandle; rownr, count: Integer; primsolution: PFloatArray; nzindex: PIntArray): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_constr_value';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_constr_value) then
    Result := Fget_constr_value(lp, rownr, count, primsolution, nzindex);
end;
{$ENDIF}

function set_unbounded(lp: THandle; colnr: Integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_unbounded';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_unbounded) then
    Result := Fset_unbounded(lp, colnr);
end;
{$ENDIF}

function is_unbounded(lp: THandle; colnr: Integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_unbounded';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_unbounded) then
    Result := Fis_unbounded(lp, colnr);
end;
{$ENDIF}

function get_basis(lp: THandle; bascolumn: PIntArray; nonbasic: boolean): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_basis';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_basis) then
    Result := Fget_basis(lp, bascolumn, nonbasic);
end;
{$ENDIF}

function set_basisvar(lp: THandle; basisPos, enteringCol: Integer): Integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_basisvar';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fset_basisvar) then
    Result := Fset_basisvar(lp, basisPos, enteringCol);
end;
{$ENDIF}

procedure put_bb_nodefunc(lp: THandle; newnode: lphandleint_intfunc; bbnodehandle: Pointer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'put_bb_nodefunc';
{$ELSE}
begin
  if Assigned(Fput_bb_nodefunc) then
    Fput_bb_nodefunc(lp, newnode, bbnodehandle);
end;
{$ENDIF}

procedure put_bb_branchfunc(lp: THandle; newbranch: lphandleint_intfunc; bbbranchhandle: Pointer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'put_bb_branchfunc';
{$ELSE}
begin
  if Assigned(Fput_bb_branchfunc) then
    Fput_bb_branchfunc(lp, newbranch, bbbranchhandle);
end;
{$ENDIF}

function write_params(lp: THandle; filename, options: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'write_params';
{$ELSE}
begin
  Result := false;
  if Assigned(Fwrite_params) then
    Result := Fwrite_params(lp, filename, options);
end;
{$ENDIF}

function read_params(lp: THandle; filename, options: PAnsiChar): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'read_params';
{$ELSE}
begin
  Result := false;
  if Assigned(Fread_params) then
    Result := Fread_params(lp, filename, options);
end;
{$ENDIF}

procedure reset_params(lp: THandle); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'reset_params';
{$ELSE}
begin
  if Assigned(Freset_params) then
    Freset_params(lp);
end;
{$ENDIF}

function set_epslevel(lp: THandle; epslevel: Integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_epslevel';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_epslevel) then
    Result := Fset_epslevel(lp, epslevel);
end;
{$ENDIF}

function set_pseudocosts(lp: THandle; clower: PFloatArray; cupper: PFloatArray; updatelimit: PIntArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_pseudocosts';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_pseudocosts) then
    Result := Fset_pseudocosts(lp, clower, cupper, updatelimit);
end;
{$ENDIF}

function get_pseudocosts(lp: THandle; clower: PFloatArray; cupper: PFloatArray; updatelimit: PIntArray): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_pseudocosts';
{$ELSE}
begin
  Result := false;
  if Assigned(Fget_pseudocosts) then
    Result := Fget_pseudocosts(lp, clower, cupper, updatelimit);
end;
{$ENDIF}

procedure set_presolve(lp: THandle; presolvemode, maxloops: Integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_presolve';
{$ELSE}
begin
  if Assigned(Fset_presolve) then
    Fset_presolve(lp, presolvemode, maxloops);
end;
{$ENDIF}

function get_presolveloops(lp: THandle): Integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_presolveloops';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_presolveloops) then
    Result := Fget_presolveloops(lp);
end;
{$ENDIF}

procedure put_abortfunc(lp: THandle; newctrlc: lphandle_intfunc; ctrlchandle: Pointer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'put_abortfunc';
{$ELSE}
begin
  if Assigned(Fput_abortfunc) then
    Fput_abortfunc(lp, newctrlc, ctrlchandle);
end;
{$ENDIF}

procedure put_logfunc(lp: THandle; newlog: lphandlestr_func; loghandle: Pointer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'put_logfunc';
{$ELSE}
begin
  if Assigned(Fput_logfunc) then
    Fput_logfunc(lp, newlog, loghandle);
end;
{$ENDIF}

procedure put_msgfunc(lp: THandle; newmsg: lphandleint_func; msghandle: Pointer; mask: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'put_msgfunc';
{$ELSE}
begin
  if Assigned(Fput_msgfunc) then
    Fput_msgfunc(lp, newmsg, msghandle, mask);
end;
{$ENDIF}

function get_rowex(lp: THandle; rownr: Integer; row: PFloatArray; colno: PIntArray): integer; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_rowex';
{$ELSE}
begin
  Result := 0;
  if Assigned(Fget_rowex) then
    Result := Fget_rowex(lp, rownr, row, colno);
end;
{$ENDIF}

function is_use_names(lp: THandle; isrow: boolean): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_use_names';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_use_names) then
    Result := Fis_use_names(lp, isrow);
end;
{$ENDIF}

procedure set_use_names(lp: THandle; isrow, use_names: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_use_names';
{$ELSE}
begin
  if Assigned(Fset_use_names) then
    Fset_use_names(lp, isrow, use_names);
end;
{$ENDIF}

function is_obj_in_basis(lp: THandle): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_obj_in_basis';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_obj_in_basis) then
    Result := Fis_obj_in_basis(lp);
end;
{$ENDIF}

procedure set_obj_in_basis(lp: THandle; obj_in_basis: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_obj_in_basis';
{$ELSE}
begin
  if Assigned(Fset_obj_in_basis) then
    Fset_obj_in_basis(lp, obj_in_basis);
end;
{$ENDIF}

function get_accuracy(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_accuracy';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_accuracy) then
    Result := Fget_accuracy(lp);
end;
{$ENDIF}

function get_break_numeric_accuracy(lp: THandle): double; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_break_numeric_accuracy';
{$ELSE}
begin
  Result := 0.0;
  if Assigned(Fget_break_numeric_accuracy) then
    Result := Fget_break_numeric_accuracy(lp);
end;
{$ENDIF}

procedure set_break_numeric_accuracy(lp: THandle;Value: double); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_break_numeric_accuracy';
{$ELSE}
begin
  if Assigned(Fset_break_numeric_accuracy) then
    Fset_break_numeric_accuracy(lp, Value);
end;
{$ENDIF}
{$ELSE}
function set_free(lp: THandle; column: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_free';
{$ELSE}
begin
  Result := false;
  if Assigned(Fset_free) then
    Result := Fset_free(lp, column);
end;
{$ENDIF}

function is_free(lp: THandle; column: integer): boolean; stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'is_free';
{$ELSE}
begin
  Result := false;
  if Assigned(Fis_free) then
    Result := Fis_free(lp, column);
end;
{$ENDIF}

procedure set_presolve(lp: THandle; do_presolve: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'set_presolve';
{$ELSE}
begin
  if Assigned(Fset_presolve) then
    Fset_presolve(lp, do_presolve);
end;
{$ENDIF}

procedure put_abortfunc(lp: THandle; newctrlc: ctrlcfunc; ctrlchandle: Pointer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'put_abortfunc';
{$ELSE}
begin
  if Assigned(Fput_abortfunc) then
    Fput_abortfunc(lp, newctrlc, ctrlchandle);
end;
{$ENDIF}

procedure put_logfunc(lp: THandle; newlog: logfunc; loghandle: Pointer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'put_logfunc';
{$ELSE}
begin
  if Assigned(Fput_logfunc) then
    Fput_logfunc(lp, newlog, loghandle);
end;
{$ENDIF}

procedure put_msgfunc(lp: THandle; newmsg: msgfunc; msghandle: Pointer; mask: integer); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'put_msgfunc';
{$ELSE}
begin
  if Assigned(Fput_msgfunc) then
    Fput_msgfunc(lp, newmsg, msghandle, mask);
end;
{$ENDIF}

procedure get_basis(lp: THandle; bascolumn: PIntArray; nonbasic: boolean); stdcall;
{$IFNDEF LPSOLVE_DYNAMIC}
external LPSOLVELIB name 'get_basis';
{$ELSE}
begin
  if Assigned(Fget_basis) then
    Fget_basis(lp, bascolumn, nonbasic);
end;
{$ENDIF}

{$ENDIF}

{$IFDEF LPSOLVE_DYNAMIC}

procedure _GetProcAddress(var lproc: Pointer; const lProcName: AnsiString);
begin
  if FLibHandle = 0 then
    lproc := nil
  else
    {$IFDEF UNIX}
    lproc := dlsym(FLibHandle, PAnsiChar(lProcName));
    {$ELSE}
    lproc := GetProcAddress(FLibHandle, PAnsiChar(lProcName));
    {$ENDIF}
end;

function LoadLPSolve: Boolean;
begin
   if FLibHandle <> 0 then
  begin
    Result := True;
    Exit;
  end;

  {$IFDEF UNIX}
  FLibHandle := dlopen(PAnsiChar(AnsiString(LPSOLVELIB)), RTLD_LAZY);
  {$ELSE}
  FLibHandle := LoadLibrary(PChar(LPSOLVELIB));
  {$ENDIF}

  Result := (FLibHandle <> 0);
  if not Result then
  begin
    raise Exception.CreateFmt( 'Fehler beim Laden der Bibliothek "%s"', [LPSOLVELIB]);
  end;

  _GetProcAddress(@Flp_solve_version, 'lp_solve_version');
  _GetProcAddress(@Fmake_lp, 'make_lp');
  _GetProcAddress(@Fresize_lp, 'resize_lp');
  _GetProcAddress(@Fget_status, 'get_status');
  _GetProcAddress(@Fget_statustext, 'get_statustext');
  _GetProcAddress(@Fdelete_lp, 'delete_lp');
  _GetProcAddress(@Ffree_lp, 'free_lp');
  _GetProcAddress(@Fset_lp_name, 'set_lp_name');
  _GetProcAddress(@Fget_lp_name, 'get_lp_name');
  _GetProcAddress(@Fhas_BFP, 'has_BFP');
  _GetProcAddress(@Fis_nativeBFP, 'is_nativeBFP');
  _GetProcAddress(@Fset_BFP, 'set_BFP');
  _GetProcAddress(@Fread_XLI, 'read_XLI');
  _GetProcAddress(@Fwrite_XLI, 'write_XLI');
  _GetProcAddress(@Fhas_XLI, 'has_XLI');
  _GetProcAddress(@Fis_nativeXLI, 'is_nativeXLI');
  _GetProcAddress(@Fset_XLI, 'set_XLI');
  _GetProcAddress(@Fset_obj, 'set_obj');
  _GetProcAddress(@Fset_obj_fn, 'set_obj_fn');
  _GetProcAddress(@Fset_obj_fnex, 'set_obj_fnex');
  _GetProcAddress(@Fstr_set_obj_fn, 'str_set_obj_fn');
  _GetProcAddress(@Fset_sense, 'set_sense');
  _GetProcAddress(@Fset_maxim, 'set_maxim');
  _GetProcAddress(@Fset_minim, 'set_minim');
  _GetProcAddress(@Fis_maxim, 'is_maxim');
  _GetProcAddress(@Fadd_constraint, 'add_constraint');
  _GetProcAddress(@Fadd_constraintex, 'add_constraintex');
  _GetProcAddress(@Fset_add_rowmode, 'set_add_rowmode');
  _GetProcAddress(@Fis_add_rowmode, 'is_add_rowmode');
  _GetProcAddress(@Fstr_add_constraint, 'str_add_constraint');
  _GetProcAddress(@Fset_row, 'set_row');
  _GetProcAddress(@Fset_rowex, 'set_rowex');
  _GetProcAddress(@Fget_row, 'get_row');
  _GetProcAddress(@Fdel_constraint, 'del_constraint');
  _GetProcAddress(@Fadd_lag_con, 'add_lag_con');
  _GetProcAddress(@Fstr_add_lag_con, 'str_add_lag_con');
  _GetProcAddress(@Fset_lag_trace, 'set_lag_trace');
  _GetProcAddress(@Fis_lag_trace, 'is_lag_trace');
  _GetProcAddress(@Fset_constr_type, 'set_constr_type');
  _GetProcAddress(@Fget_constr_type, 'get_constr_type');
  _GetProcAddress(@Fis_constr_type, 'is_constr_type');
  _GetProcAddress(@Fset_rh, 'set_rh');
  _GetProcAddress(@Fget_rh, 'get_rh');
  _GetProcAddress(@Fset_rh_range, 'set_rh_range');
  _GetProcAddress(@Fget_rh_range, 'get_rh_range');
  _GetProcAddress(@Fset_rh_vec, 'set_rh_vec');
  _GetProcAddress(@Fstr_set_rh_vec, 'str_set_rh_vec');
  _GetProcAddress(@Fadd_column, 'add_column');
  _GetProcAddress(@Fadd_columnex, 'add_columnex');
  _GetProcAddress(@Fstr_add_column, 'str_add_column');
  _GetProcAddress(@Fset_column, 'set_column');
  _GetProcAddress(@Fset_columnex, 'set_columnex');
  _GetProcAddress(@Fcolumn_in_lp, 'column_in_lp');
  _GetProcAddress(@Fget_column, 'get_column');
  _GetProcAddress(@Fdel_column, 'del_column');
  _GetProcAddress(@Fset_mat, 'set_mat');
  _GetProcAddress(@Fget_mat, 'get_mat');
  _GetProcAddress(@Fget_mat_byindex, 'get_mat_byindex');
  _GetProcAddress(@Fget_nonzeros, 'get_nonzeros');
  _GetProcAddress(@Fset_bounds_tighter, 'set_bounds_tighter');
  _GetProcAddress(@Fget_bounds_tighter, 'get_bounds_tighter');
  _GetProcAddress(@Fset_upbo, 'set_upbo');
  _GetProcAddress(@Fget_upbo, 'get_upbo');
  _GetProcAddress(@Fset_lowbo, 'set_lowbo');
  _GetProcAddress(@Fget_lowbo, 'get_lowbo');
  _GetProcAddress(@Fset_bounds, 'set_bounds');
  _GetProcAddress(@Fset_int, 'set_int');
  _GetProcAddress(@Fis_int, 'is_int');
  _GetProcAddress(@Fset_binary, 'set_binary');
  _GetProcAddress(@Fis_binary, 'is_binary');
  _GetProcAddress(@Fset_semicont, 'set_semicont');
  _GetProcAddress(@Fis_semicont, 'is_semicont');
  _GetProcAddress(@Fis_negative, 'is_negative');
  _GetProcAddress(@Fset_var_weights, 'set_var_weights');
  _GetProcAddress(@Fget_var_priority, 'get_var_priority');
  _GetProcAddress(@Fadd_SOS, 'add_SOS');
  _GetProcAddress(@Fis_SOS_var, 'is_SOS_var');
  _GetProcAddress(@Fset_row_name, 'set_row_name');
  _GetProcAddress(@Fget_row_name, 'get_row_name');
  _GetProcAddress(@Fget_origrow_name, 'get_origrow_name');
  _GetProcAddress(@Fset_col_name, 'set_col_name');
  _GetProcAddress(@Fget_col_name, 'get_col_name');
  _GetProcAddress(@Fget_origcol_name, 'get_origcol_name');
  _GetProcAddress(@Funscale, 'unscale');
  _GetProcAddress(@Fset_preferdual, 'set_preferdual');
  _GetProcAddress(@Fset_simplextype, 'set_simplextype');
  _GetProcAddress(@Fget_simplextype, 'get_simplextype');
  _GetProcAddress(@Fdefault_basis, 'default_basis');
  _GetProcAddress(@Fset_basiscrash, 'set_basiscrash');
  _GetProcAddress(@Fget_basiscrash, 'get_basiscrash');
  _GetProcAddress(@Fset_basis, 'set_basis');
  _GetProcAddress(@Fis_feasible, 'is_feasible');
  _GetProcAddress(@Fsolve, 'solve');
  _GetProcAddress(@Ftime_elapsed, 'time_elapsed');
  _GetProcAddress(@Fget_primal_solution, 'get_primal_solution');
  _GetProcAddress(@Fget_ptr_primal_solution, 'get_ptr_primal_solution');
  _GetProcAddress(@Fget_dual_solution, 'get_dual_solution');
  _GetProcAddress(@Fget_ptr_dual_solution, 'get_ptr_dual_solution');
  _GetProcAddress(@Fget_lambda, 'get_lambda');
  _GetProcAddress(@Fget_ptr_lambda, 'get_ptr_lambda');
  _GetProcAddress(@Freset_basis, 'reset_basis');
  _GetProcAddress(@Fread_MPS_file, 'read_MPS');
  _GetProcAddress(@Fread_mps_stream, 'read_mps');
  _GetProcAddress(@Fwrite_mps_file, 'write_mps');
  _GetProcAddress(@Fwrite_MPS_stream, 'write_MPS');
  _GetProcAddress(@Fread_freeMPS_file, 'read_freeMPS');
  _GetProcAddress(@Fread_freemps_stream, 'read_freemps');
  _GetProcAddress(@Fwrite_freemps_file, 'write_freemps');
  _GetProcAddress(@Fwrite_freeMPS_stream, 'write_freeMPS');
  _GetProcAddress(@Fguess_basis, 'guess_basis');
  _GetProcAddress(@Fread_basis, 'read_basis');
  _GetProcAddress(@Fwrite_basis, 'write_basis');
  _GetProcAddress(@Fwrite_lp_file, 'write_lp');
  _GetProcAddress(@Fwrite_LP_stream, 'write_LP');
  _GetProcAddress(@Fread_lp_stream, 'read_lp');
  _GetProcAddress(@Fread_LP_file, 'read_LP');
  _GetProcAddress(@Fprint_lp, 'print_lp');
  _GetProcAddress(@Fprint_tableau, 'print_tableau');
  _GetProcAddress(@Fprint_objective, 'print_objective');
  _GetProcAddress(@Fprint_solution, 'print_solution');
  _GetProcAddress(@Fprint_constraints, 'print_constraints');
  _GetProcAddress(@Fprint_duals, 'print_duals');
  _GetProcAddress(@Fprint_scales, 'print_scales');
  _GetProcAddress(@Fprint_str, 'print_str');
  _GetProcAddress(@Fset_outputstream, 'set_outputstream');
  _GetProcAddress(@Fset_outputfile, 'set_outputfile');
  _GetProcAddress(@Fset_verbose, 'set_verbose');
  _GetProcAddress(@Fget_verbose, 'get_verbose');
  _GetProcAddress(@Fset_timeout, 'set_timeout');
  _GetProcAddress(@Fget_timeout, 'get_timeout');
  _GetProcAddress(@Fset_print_sol, 'set_print_sol');
  _GetProcAddress(@Fget_print_sol, 'get_print_sol');
  _GetProcAddress(@Fset_debug, 'set_debug');
  _GetProcAddress(@Fis_debug, 'is_debug');
  _GetProcAddress(@Fset_trace, 'set_trace');
  _GetProcAddress(@Fis_trace, 'is_trace');
  _GetProcAddress(@Fprint_debugdump, 'print_debugdump');
  _GetProcAddress(@Fset_anti_degen, 'set_anti_degen');
  _GetProcAddress(@Fget_anti_degen, 'get_anti_degen');
  _GetProcAddress(@Fis_anti_degen, 'is_anti_degen');
  _GetProcAddress(@Fget_presolve, 'get_presolve');
  _GetProcAddress(@Fis_presolve, 'is_presolve');
  _GetProcAddress(@Fget_orig_index, 'get_orig_index');
  _GetProcAddress(@Fget_lp_index, 'get_lp_index');
  _GetProcAddress(@Fset_maxpivot, 'set_maxpivot');
  _GetProcAddress(@Fget_maxpivot, 'get_maxpivot');
  _GetProcAddress(@Fset_obj_bound, 'set_obj_bound');
  _GetProcAddress(@Fget_obj_bound, 'get_obj_bound');
  _GetProcAddress(@Fset_mip_gap, 'set_mip_gap');
  _GetProcAddress(@Fget_mip_gap, 'get_mip_gap');
  _GetProcAddress(@Fset_bb_rule, 'set_bb_rule');
  _GetProcAddress(@Fget_bb_rule, 'get_bb_rule');
  _GetProcAddress(@Fset_var_branch, 'set_var_branch');
  _GetProcAddress(@Fget_var_branch, 'get_var_branch');
  _GetProcAddress(@Fis_infinite, 'is_infinite');
  _GetProcAddress(@Fset_infinite, 'set_infinite');

  _GetProcAddress(@Fget_infinite, 'get_infinite');
  _GetProcAddress(@Fset_epsint, 'set_epsint');
  _GetProcAddress(@Fget_epsint, 'get_epsint');
  _GetProcAddress(@Fset_epsb, 'set_epsb');
  _GetProcAddress(@Fget_epsb, 'get_epsb');
  _GetProcAddress(@Fset_epsd, 'set_epsd');
  _GetProcAddress(@Fget_epsd, 'get_epsd');
  _GetProcAddress(@Fset_epsel, 'set_epsel');
  _GetProcAddress(@Fget_epsel, 'get_epsel');
  _GetProcAddress(@Fset_scaling, 'set_scaling');
  _GetProcAddress(@Fget_scaling, 'get_scaling');
  _GetProcAddress(@Fis_scalemode, 'is_scalemode');
  _GetProcAddress(@Fis_scaletype, 'is_scaletype');
  _GetProcAddress(@Fis_integerscaling, 'is_integerscaling');
  _GetProcAddress(@Fset_scalelimit, 'set_scalelimit');
  _GetProcAddress(@Fget_scalelimit, 'get_scalelimit');
  _GetProcAddress(@Fset_improve, 'set_improve');
  _GetProcAddress(@Fget_improve, 'get_improve');
  _GetProcAddress(@Fset_pivoting, 'set_pivoting');
  _GetProcAddress(@Fget_pivoting, 'get_pivoting');
  _GetProcAddress(@Fset_partialprice, 'set_partialprice');
  _GetProcAddress(@Fget_partialprice, 'get_partialprice');
  _GetProcAddress(@Fset_multiprice, 'set_multiprice');
  _GetProcAddress(@Fget_multiprice, 'get_multiprice');
  _GetProcAddress(@Fis_piv_mode, 'is_piv_mode');
  _GetProcAddress(@Fis_piv_rule, 'is_piv_rule');
  _GetProcAddress(@Fset_break_at_first, 'set_break_at_first');
  _GetProcAddress(@Fis_break_at_first, 'is_break_at_first');
  _GetProcAddress(@Fset_bb_floorfirst, 'set_bb_floorfirst');
  _GetProcAddress(@Fget_bb_floorfirst, 'get_bb_floorfirst');
  _GetProcAddress(@Fset_bb_depthlimit, 'set_bb_depthlimit');
  _GetProcAddress(@Fget_bb_depthlimit, 'get_bb_depthlimit');
  _GetProcAddress(@Fset_break_at_value, 'set_break_at_value');
  _GetProcAddress(@Fget_break_at_value, 'get_break_at_value');
  _GetProcAddress(@Fset_negrange, 'set_negrange');
  _GetProcAddress(@Fget_negrange, 'get_negrange');
  _GetProcAddress(@Fset_epsperturb, 'set_epsperturb');
  _GetProcAddress(@Fget_epsperturb, 'get_epsperturb');
  _GetProcAddress(@Fset_epspivot, 'set_epspivot');
  _GetProcAddress(@Fget_epspivot, 'get_epspivot');
  _GetProcAddress(@Fget_max_level, 'get_max_level');
  _GetProcAddress(@Fget_total_nodes, 'get_total_nodes');
  _GetProcAddress(@Fget_total_iter, 'get_total_iter');
  _GetProcAddress(@Fget_objective, 'get_objective');
  _GetProcAddress(@Fget_working_objective, 'get_working_objective');
  _GetProcAddress(@Fget_var_primalresult, 'get_var_primalresult');
  _GetProcAddress(@Fget_var_dualresult, 'get_var_dualresult');
  _GetProcAddress(@Fget_variables, 'get_variables');
  _GetProcAddress(@Fget_ptr_variables, 'get_ptr_variables');
  _GetProcAddress(@Fget_constraints, 'get_constraints');
  _GetProcAddress(@Fget_ptr_constraints, 'get_ptr_constraints');
  _GetProcAddress(@Fget_sensitivity_rhs, 'get_sensitivity_rhs');
  _GetProcAddress(@Fget_ptr_sensitivity_rhs, 'get_ptr_sensitivity_rhs');
  _GetProcAddress(@Fget_sensitivity_obj, 'get_sensitivity_obj');
  _GetProcAddress(@Fget_sensitivity_objex, 'get_sensitivity_objex');
  _GetProcAddress(@Fget_ptr_sensitivity_obj, 'get_ptr_sensitivity_obj');
  _GetProcAddress(@Fget_ptr_sensitivity_objex, 'get_ptr_sensitivity_objex');
  _GetProcAddress(@Fset_solutionlimit, 'set_solutionlimit');
  _GetProcAddress(@Fget_solutionlimit, 'get_solutionlimit');
  _GetProcAddress(@Fget_solutioncount, 'get_solutioncount');
  _GetProcAddress(@Fget_Norig_rows, 'get_Norig_rows');
  _GetProcAddress(@Fget_Nrows, 'get_Nrows');
  _GetProcAddress(@Fget_Lrows, 'get_Lrows');
  _GetProcAddress(@Fget_Norig_columns, 'get_Norig_columns');
  _GetProcAddress(@Fget_Ncolumns, 'get_Ncolumns');
  _GetProcAddress(@Fget_nameindex, 'get_nameindex');
  {$IFDEF LPS55_UP}
  _GetProcAddress(@Fcopy_lp, 'copy_lp');
  _GetProcAddress(@Fdualize_lp, 'dualize_lp');
  _GetProcAddress(@Fget_constr_value, 'get_constr_value');
  _GetProcAddress(@Fget_columnex, 'get_columnex');
  _GetProcAddress(@Fset_unbounded, 'set_unbounded');
  _GetProcAddress(@Fis_unbounded, 'is_unbounded');
  _GetProcAddress(@Fget_basis, 'get_basis');
  _GetProcAddress(@Fset_basisvar, 'set_basisvar');
  _GetProcAddress(@Fput_bb_nodefunc, 'put_bb_nodefunc');
  _GetProcAddress(@Fput_bb_branchfunc, 'put_bb_branchfunc');
  _GetProcAddress(@Fput_abortfunc, 'put_abortfunc');
  _GetProcAddress(@Fput_logfunc, 'put_logfunc');
  _GetProcAddress(@Fput_msgfunc, 'put_msgfunc');
  _GetProcAddress(@Fwrite_params, 'write_params');
  _GetProcAddress(@Fread_params, 'read_params');
  _GetProcAddress(@Freset_params, 'reset_params');
  _GetProcAddress(@Fset_presolve, 'set_presolve');
  _GetProcAddress(@Fget_presolveloops, 'get_presolveloops');
  _GetProcAddress(@Fset_epslevel, 'set_epslevel');
  _GetProcAddress(@Fget_rowex, 'get_rowex');
  _GetProcAddress(@Fis_use_names, 'is_use_names');
  _GetProcAddress(@Fset_use_names, 'set_use_names');
  _GetProcAddress(@Fis_obj_in_basis, 'is_obj_in_basis');
  _GetProcAddress(@Fset_obj_in_basis, 'set_obj_in_basis');
  _GetProcAddress(@Fget_accuracy, 'get_accuracy');
  _GetProcAddress(@Fget_break_numeric_accuracy, 'get_break_numeric_accuracy');
  _GetProcAddress(@Fset_break_numeric_accuracy, 'set_break_numeric_accuracy');
  _GetProcAddress(@Fset_pseudocosts, 'set_pseudocosts');
  _GetProcAddress(@Fget_pseudocosts, 'get_pseudocosts');
  {$ELSE}
  _GetProcAddress(@Fset_free, 'set_free');
  _GetProcAddress(@Fis_free, 'is_free');
  _GetProcAddress(@Fget_basis, 'get_basis');
  _GetProcAddress(@Fput_abortfunc, 'put_abortfunc');
  _GetProcAddress(@Fput_logfunc, 'put_logfunc');
  _GetProcAddress(@Fput_msgfunc, 'put_msgfunc');
  _GetProcAddress(@Fset_presolve, 'set_presolve');
  {$ENDIF}
end;

procedure UnloadLPSolve;
begin
  if FLibHandle = 0 then Exit; // Bereits entladen

  {$IFDEF UNIX}
  dlclose(FLibHandle);
  {$ELSE}
  FreeLibrary(FLibHandle);
  {$ENDIF}

  FLibHandle := 0;

  // Setze alle Funktionszeiger auf nil, um Hänger zu vermeiden
  Flp_solve_version := nil;
  Fmake_lp := nil;
  Fresize_lp := nil;
  Fget_status := nil;
  Fget_statustext := nil;
  Fdelete_lp := nil;
  Ffree_lp := nil;
  Fset_lp_name := nil;
  Fget_lp_name := nil;
  Fhas_BFP := nil;
  Fis_nativeBFP := nil;
  Fset_BFP := nil;
  Fread_XLI := nil;
  Fwrite_XLI := nil;
  Fhas_XLI := nil;
  Fis_nativeXLI := nil;
  Fset_XLI := nil;
  Fset_obj := nil;
  Fset_obj_fn := nil;
  Fset_obj_fnex := nil;
  Fstr_set_obj_fn := nil;
  Fset_sense := nil;
  Fset_maxim := nil;
  Fset_minim := nil;
  Fis_maxim := nil;
  Fadd_constraint := nil;
  Fadd_constraintex := nil;
  Fset_add_rowmode := nil;
  Fis_add_rowmode := nil;
  Fstr_add_constraint := nil;
  Fset_row := nil;
  Fset_rowex := nil;
  Fget_row := nil;
  Fdel_constraint := nil;
  Fadd_lag_con := nil;
  Fstr_add_lag_con := nil;
  Fset_lag_trace := nil;
  Fis_lag_trace := nil;
  Fset_constr_type := nil;
  Fget_constr_type := nil;
  Fis_constr_type := nil;
  Fset_rh := nil;
  Fget_rh := nil;
  Fset_rh_range := nil;
  Fget_rh_range := nil;
  Fset_rh_vec := nil;
  Fstr_set_rh_vec := nil;
  Fadd_column := nil;
  Fadd_columnex := nil;
  Fstr_add_column := nil;
  Fset_column := nil;
  Fset_columnex := nil;
  Fcolumn_in_lp := nil;
  Fget_column := nil;
  Fdel_column := nil;
  Fset_mat := nil;
  Fget_mat := nil;
  Fget_mat_byindex := nil;
  Fget_nonzeros := nil;
  Fset_bounds_tighter := nil;
  Fget_bounds_tighter := nil;
  Fset_upbo := nil;
  Fget_upbo := nil;
  Fset_lowbo := nil;
  Fget_lowbo := nil;
  Fset_bounds := nil;
  Fset_int := nil;
  Fis_int := nil;
  Fset_binary := nil;
  Fis_binary := nil;
  Fset_semicont := nil;
  Fis_semicont := nil;
  Fis_negative := nil;
  Fset_var_weights := nil;
  Fget_var_priority := nil;
  Fadd_SOS := nil;
  Fis_SOS_var := nil;
  Fset_row_name := nil;
  Fget_row_name := nil;
  Fget_origrow_name := nil;
  Fset_col_name := nil;
  Fget_col_name := nil;
  Fget_origcol_name := nil;
  Funscale := nil;
  Fset_preferdual := nil;
  Fset_simplextype := nil;
  Fget_simplextype := nil;
  Fdefault_basis := nil;
  Fset_basiscrash := nil;
  Fget_basiscrash := nil;
  Fset_basis := nil;
  Fis_feasible := nil;
  Fsolve := nil;
  Ftime_elapsed := nil;
  Fget_primal_solution := nil;
  Fget_ptr_primal_solution := nil;
  Fget_dual_solution := nil;
  Fget_ptr_dual_solution := nil;
  Fget_lambda := nil;
  Fget_ptr_lambda := nil;
  Freset_basis := nil;
  Fread_MPS_file := nil;
  Fread_mps_stream := nil;
  Fwrite_mps_file := nil;
  Fwrite_MPS_stream := nil;
  Fread_freeMPS_file := nil;
  Fread_freemps_stream := nil;
  Fwrite_freemps_file := nil;
  Fwrite_freeMPS_stream := nil;
  Fguess_basis := nil;
  Fread_basis := nil;
  Fwrite_basis := nil;
  Fwrite_lp_file := nil;
  Fwrite_LP_stream := nil;
  Fread_lp_stream := nil;
  Fread_LP_file := nil;
  Fprint_lp := nil;
  Fprint_tableau := nil;
  Fprint_objective := nil;
  Fprint_solution := nil;
  Fprint_constraints := nil;
  Fprint_duals := nil;
  Fprint_scales := nil;
  Fprint_str := nil;
  Fset_outputstream := nil;
  Fset_outputfile := nil;
  Fset_verbose := nil;
  Fget_verbose := nil;
  Fset_timeout := nil;
  Fget_timeout := nil;
  Fset_print_sol := nil;
  Fget_print_sol := nil;
  Fset_debug := nil;
  Fis_debug := nil;
  Fset_trace := nil;
  Fis_trace := nil;
  Fprint_debugdump := nil;
  Fset_anti_degen := nil;
  Fget_anti_degen := nil;
  Fis_anti_degen := nil;
  Fget_presolve := nil;
  Fis_presolve := nil;
  Fget_orig_index := nil;
  Fget_lp_index := nil;
  Fset_maxpivot := nil;
  Fget_maxpivot := nil;
  Fset_obj_bound := nil;
  Fget_obj_bound := nil;
  Fset_mip_gap := nil;
  Fget_mip_gap := nil;
  Fset_bb_rule := nil;
  Fget_bb_rule := nil;
  Fset_var_branch := nil;
  Fget_var_branch := nil;
  Fis_infinite := nil;
  Fset_infinite := nil;
  Fget_infinite := nil;
  Fset_epsint := nil;
  Fget_epsint := nil;
  Fset_epsb := nil;
  Fget_epsb := nil;
  Fset_epsd := nil;
  Fget_epsd := nil;
  Fset_epsel := nil;
  Fget_epsel := nil;
  Fset_scaling := nil;
  Fget_scaling := nil;
  Fis_scalemode := nil;
  Fis_scaletype := nil;
  Fis_integerscaling := nil;
  Fset_scalelimit := nil;
  Fget_scalelimit := nil;
  Fset_improve := nil;
  Fget_improve := nil;
  Fset_pivoting := nil;
  Fget_pivoting := nil;
  Fset_partialprice := nil;
  Fget_partialprice := nil;
  Fset_multiprice := nil;
  Fget_multiprice := nil;
  Fis_piv_mode := nil;
  Fis_piv_rule := nil;
  Fset_break_at_first := nil;
  Fis_break_at_first := nil;
  Fset_bb_floorfirst := nil;
  Fget_bb_floorfirst := nil;
  Fset_bb_depthlimit := nil;
  Fget_bb_depthlimit := nil;
  Fset_break_at_value := nil;
  Fget_break_at_value := nil;
  Fset_negrange := nil;
  Fget_negrange := nil;
  Fset_epsperturb := nil;
  Fget_epsperturb := nil;
  Fset_epspivot := nil;
  Fget_epspivot := nil;
  Fget_max_level := nil;
  Fget_total_nodes := nil;
  Fget_total_iter := nil;
  Fget_objective := nil;
  Fget_working_objective := nil;
  Fget_var_primalresult := nil;
  Fget_var_dualresult := nil;
  Fget_variables := nil;
  Fget_ptr_variables := nil;
  Fget_constraints := nil;
  Fget_ptr_constraints := nil;
  Fget_sensitivity_rhs := nil;
  Fget_ptr_sensitivity_rhs := nil;
  Fget_sensitivity_obj := nil;
  Fget_sensitivity_objex := nil;
  Fget_ptr_sensitivity_obj := nil;
  Fget_ptr_sensitivity_objex := nil;
  Fset_solutionlimit := nil;
  Fget_solutionlimit := nil;
  Fget_solutioncount := nil;
  Fget_Norig_rows := nil;
  Fget_Nrows := nil;
  Fget_Lrows := nil;
  Fget_Norig_columns := nil;
  Fget_Ncolumns := nil;
  Fget_nameindex := nil;
  {$IFDEF LPS55_UP}
  Fcopy_lp := nil;
  Fdualize_lp := nil;
  Fget_constr_value := nil;
  Fget_columnex := nil;
  Fset_unbounded := nil;
  Fis_unbounded := nil;
  Fget_basis := nil;
  Fset_basisvar := nil;
  Fput_bb_nodefunc := nil;
  Fput_bb_branchfunc := nil;
  Fput_abortfunc := nil;
  Fput_logfunc := nil;
  Fput_msgfunc := nil;
  Fwrite_params := nil;
  Fread_params := nil;
  Freset_params := nil;
  Fset_presolve := nil;
  Fget_presolveloops := nil;
  Fset_epslevel := nil;
  Fget_rowex := nil;
  Fis_use_names := nil;
  Fset_use_names := nil;
  Fis_obj_in_basis := nil;
  Fset_obj_in_basis := nil;
  Fget_accuracy := nil;
  Fget_break_numeric_accuracy := nil;
  Fset_break_numeric_accuracy := nil;
  Fset_pseudocosts := nil;
  Fget_pseudocosts := nil;
  {$ELSE}
  Fset_free := nil;
  Fis_free := nil;
  Fget_basis := nil;
  Fput_abortfunc := nil;
  Fput_logfunc := nil;
  Fput_msgfunc := nil;
  Fset_presolve := nil;
  {$ENDIF}
end;

function IsLPSolveLoaded: Boolean;
begin
  Result := (FLibHandle <> 0);
end;

{$ENDIF}

initialization

finalization
{$IFDEF LPSOLVE_DYNAMIC}
  UnloadLPSolve;
{$ENDIF}

end.
