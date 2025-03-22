/*
   This file is an automatically generated lp for 'lp_solve (Version 5)'.
       (Constraints for   : "minipendulum.art3")
       (Filename should be: "constraints.lp.Box[regu]")
       (Processing Date   : Tue Mar 24 22:50:57 GMT 2009)

   Contains 66 proper constraints over 79 variables.
       (plus 8 trivial constraints)

   Resource Metric:
    WCET for Renesas M32, based on AbsInt's aiT.
    Worst-case execution times for BOXED HAM-Code execution, via C through IAR-Compiler.
    Selection codes: HumeTimeM32, TimeM32, T32, Time, RenesasM32, HumeTimeRenesasM32, HumeTRM32
    Monotone: True
    Revision: 1.1.2.79
    Date: 2009/03/10 15:57:50
    Author: jost

   Annotated Type of main expression:

@BOX: regu
  regu.b1:    wire[W<q092>:tuple[T]|NOVAL]
  regu.b2:    wire[W<q093>:tuple[T]|NOVAL]
  regu.b3:    wire[W<q094>:tuple[T]|NOVAL]
  regu.clk:   wire[W<q095>:float|NOVAL]
  regu.alpha_w:
              wire[W<q096>:word|NOVAL]
  regu.theta_w:
              wire[W<q097>:word|NOVAL]
  regu.etat:  wire[W<q098>:tuple[T:bool,bool,float,float,float,word,word]|NOVAL]
---a099/o100--->
  regu.moteur_w:
              wire[W<p088>:word|NOVAL]
  regu.erreur_w:
              wire[W<p089>:word|NOVAL]
  regu.compteur_w:
              wire[W<p090>:word|NOVAL]
  regu.etat': wire[W<p091>:tuple[T:bool,bool,float,float,float,word,word]|NOVAL]

 */

Min: -p088 -p089 -p090 -p091 +2 q092 +2 q093 +2 q094 +2 q095 +2 q096 +2 q097 +2 q098 +3 a099 -o100;

/*
  L0039C002 Function 'run_etat_LIFTED':  (word) -(C004/C005)-> wire[W<b006>:tuple[T:bool,bool,float,float,float,word,word]|NOVAL]
 */
/*
  L0039C002:
    ?wire1tuple7boolboolfloatfloatfloatwordword[<d023>:?tuple7boolboolfloatfloatfloatwordword[<0>:bool,bool,float,float,float,word,word]|*]
   subtypes
    ?wire1tuple7boolboolfloatfloatfloatwordword[<b006>:?tuple7boolboolfloatfloatfloatwordword[<0>:bool,bool,float,float,float,word,word]|*]
 */
L0039C002__Sub: -d023 +b006 <= 0;
L0041C010_Mmi1: -m011 +m007 <= 0;
L0041C010_Mmi2: +m007 +x008 -m011 <= 0;
L0041C010_Msu1: -x004 +m007 +x008 +x009 -x014 <= 0;
L0041C010_Msu2: -x004 +m011 <= 0;
L0041C010_Mtly: +m007 +x008 -m010 -x013 <= 0;
L0041C010_Pbfr: +20 -x008 +x012 <= 0;
L0041C010_Pvar: +36 -x012 +x013 <= 0;
L0041C010__Pok: +10 -x013 +x014 <= 0;
L0043C016_MkHe: +83 -x009 +z015 <= 0;
L0044C016__Var: +39 -z015 +z016 <= 0;
L0045C016_MkFl: +91 -z016 +z017 <= 0;
L0046C016_MkFl: +91 -z017 +z018 <= 0;
L0047C016_MkFl: +91 -z018 +z019 <= 0;
L0053C010__Con: +442 -z021 +z022 <= 0;
L0054C009__Con: +y005 -z022 +d023 <= 0;
/*
  L0099C001:
    ?bundle4wordwordwordtuple7boolboolfloatfloatfloatwordword[*:?wire1word[<d143>:word|*],?wire1word[<d144>:word|*],?wire1word[<d145>:word|*],?wire1tuple7boolboolfloatfloatfloatwordword[<d146>:?tuple7boolboolfloatfloatfloatwordword[<0>:bool,bool,float,float,float,word,word]|*]]
   subtypes
    ?bundle4wordwordwordtuple7boolboolfloatfloatfloatwordword[*:?wire1word[<p088>:word|*],?wire1word[<p089>:word|*],?wire1word[<p090>:word|*],?wire1tuple7boolboolfloatfloatfloatwordword[<p091>:?tuple7boolboolfloatfloatfloatwordword[<0>:bool,bool,float,float,float,word,word]|*]]
 */
L0099C001_Init: +581 -a099 +a101 <= 0;
L0099C001__Sub: -d143 +p088 <= 0;
L0099C001__Sub: -d144 +p089 <= 0;
L0099C001__Sub: -d145 +p090 <= 0;
L0099C001__Sub: -d146 +p091 <= 0;
L0103C026__MBf: +72 +m102 -m103 <= 0;
L0104C361_Mmi1: -m107 +m102 <= 0;
L0104C361_Mmi2: +m103 +a104 -m107 <= 0;
L0104C361_Msu1: -q092 -q095 -q096 -q097 -q098 -a101 +m103 +a104 +a105 -a133 <= 0;
L0104C361_Msu2: -q092 -q095 -q096 -q097 -q098 -a101 +m107 <= 0;
L0104C361_Mtly: +m103 +a104 -m106 -a132 <= 0;
L0104C361_PCaf: +32 -a129 +a131 <= 0;
L0104C361_PCaf: +9 -a128 +a129 <= 0;
L0104C361_PCaf: +32 -a118 +a119 <= 0;
L0104C361_PCaf: +32 -a116 +a117 <= 0;
L0104C361_PCaf: +32 -a114 +a115 <= 0;
L0104C361_PCaf: +32 -a110 +a111 <= 0;
L0104C361_PCbf: +55 -a108 +a109 <= 0;
L0104C361_PCbf: +11 -a119 +a120 <= 0;
L0104C361_PCbf: +11 -a112 +a113 <= 0;
L0104C361_PCbf: +11 -a111 +a112 <= 0;
L0104C361_PCfl: +32 -a130 +a132 <= 0;
L0104C361_PCfl: +9 -a123 +a130 <= 0;
L0104C361_PCmi: +394 -a120 +a121 <= 0;
L0104C361_Pbfr: +20 -a104 +a108 <= 0;
L0104C361_Pval: +35 -a122 +a123 <= 0;
L0104C361_Pval: +35 -a121 +a122 <= 0;
L0104C361_Pvar: +36 -a127 +a128 <= 0;
L0104C361_Pvar: +36 -a126 +a127 <= 0;
L0104C361_Pvar: +36 -a125 +a126 <= 0;
L0104C361_Pvar: +36 -a124 +a125 <= 0;
L0104C361_Pvar: +36 -a123 +a124 <= 0;
L0104C361_Pvar: +36 -a117 +a118 <= 0;
L0104C361_Pvar: +36 -a115 +a116 <= 0;
L0104C361_Pvar: +36 -a113 +a114 <= 0;
L0104C361_Pvar: +36 -a109 +a110 <= 0;
L0104C361__Pok: +10 -a131 +a133 <= 0;
L0106C017__Var: +39 -a105 +z134 <= 0;
/*
  L0108C011: Application 'run_etat_LIFTED'  V{20}.P(39)
  (word) -(C004/C005)-> wire[W<b006>:tuple[T:bool,bool,float,float,float,word,word]|NOVAL]
 */
L0108C011_MkBo: +85 -z020 +z021 <= 0;
L0108C011_MkBo: +85 -z019 +z020 <= 0;
L0108C011__Api: +142 +x004 -z134 +e136 <= 0;
L0108C011__Apo: +169 -y005 +z135 -e136 <= 0;
L0109C018__Con: +25 -z135 +z137 <= 0;
L0110C018__Con: +25 -z137 +z139 <= 0;
L0111C018__Con: +25 -z139 +z141 <= 0;
/*
  L0113C007:
    ?wire1tuple7boolboolfloatfloatfloatwordword[<b006>:?tuple7boolboolfloatfloatfloatwordword[<0>:bool,bool,float,float,float,word,word]|*]
   subtypes
    ?wire1tuple7boolboolfloatfloatfloatwordword[<d146>:?tuple7boolboolfloatfloatfloatwordword[<0>:bool,bool,float,float,float,word,word]|*]
 */
/*
  L0113C007:
    ?wire1word[<d138>:word|*]
   subtypes
    ?wire1word[<d145>:word|*]
 */
/*
  L0113C007:
    ?wire1word[<d140>:word|*]
   subtypes
    ?wire1word[<d144>:word|*]
 */
/*
  L0113C007:
    ?wire1word[<d142>:word|*]
   subtypes
    ?wire1word[<d143>:word|*]
 */
L0113C007__Con: +286 +o100 -z141 <= 0;
L0113C007__Sub: -b006 +d146 <= 0;
L0113C007__Sub: -d138 +d145 <= 0;
L0113C007__Sub: -d140 +d144 <= 0;
L0113C007__Sub: -d142 +d143 <= 0;
0 <= p088 <= 1000000;
0 <= p089 <= 1000000;
0 <= p090 <= 1000000;
0 <= p091 <= 1000000;
0 <= o100 <= 1000000;
x004 >= 0;
y005 >= 0;
b006 >= 0;
m007 >= 0;
x008 >= 0;
x009 >= 0;
m010 >= 0;
m011 >= 0;
x012 >= 0;
x013 >= 0;
x014 >= 0;
z015 >= 0;
z016 >= 0;
z017 >= 0;
z018 >= 0;
z019 >= 0;
z020 >= 0;
z021 >= 0;
z022 >= 0;
d023 >= 0;
q092 >= 0;
q093 >= 0;
q094 >= 0;
q095 >= 0;
q096 >= 0;
q097 >= 0;
q098 >= 0;
a099 >= 0;
a101 >= 0;
m102 >= 0;
m103 >= 0;
a104 >= 0;
a105 >= 0;
m106 >= 0;
m107 >= 0;
a108 >= 0;
a109 >= 0;
a110 >= 0;
a111 >= 0;
a112 >= 0;
a113 >= 0;
a114 >= 0;
a115 >= 0;
a116 >= 0;
a117 >= 0;
a118 >= 0;
a119 >= 0;
a120 >= 0;
a121 >= 0;
a122 >= 0;
a123 >= 0;
a124 >= 0;
a125 >= 0;
a126 >= 0;
a127 >= 0;
a128 >= 0;
a129 >= 0;
a130 >= 0;
a131 >= 0;
a132 >= 0;
a133 >= 0;
z134 >= 0;
z135 >= 0;
e136 >= 0;
z137 >= 0;
d138 >= 0;
z139 >= 0;
d140 >= 0;
z141 >= 0;
d142 >= 0;
d143 >= 0;
d144 >= 0;
d145 >= 0;
d146 >= 0;
