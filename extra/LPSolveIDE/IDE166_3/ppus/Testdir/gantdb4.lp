//Example With 4 Teams And 2 Rounds

// Objective function
min: ;
 
// same number of games in each round
 + r1_3_4 + r1_2_4 + r1_2_3 + r1_1_4 + r1_1_3 + r1_1_2 = 2;
 + r2_3_4 + r2_2_4 + r2_2_3 + r2_1_4 + r2_1_3 + r2_1_2 = 2;
 
// relate rounds to games
 + r2_1_2 + r1_1_2 = g1_2;
 + r2_1_3 + r1_1_3 = g1_3;
 + r2_1_4 + r1_1_4 = g1_4;
 + r2_2_3 + r1_2_3 = g2_3;
 + r2_2_4 + r1_2_4 = g2_4;
 + r2_3_4 + r1_3_4 = g3_4;
 
// each team plays once in each round
 + r1_1_4 + r1_1_3 + r1_1_2 <= 1;
 + r1_2_4 + r1_2_3 + r1_1_2 <= 1;
 + r1_3_4 + r1_2_3 + r1_1_3 <= 1;
 + r1_3_4 + r1_2_4 + r1_1_4 <= 1;
 + r2_1_4 + r2_1_3 + r2_1_2 <= 1;
 + r2_2_4 + r2_2_3 + r2_1_2 <= 1;
 + r2_3_4 + r2_2_3 + r2_1_3 <= 1;
 + r2_3_4 + r2_2_4 + r2_1_4 <= 1;
 
// edges per vertex (number of games each team will play)
 + g1_2 + g1_3 + g1_4 <= 2;
 + g1_2 + g2_3 + g2_4 <= 2;
 + g1_3 + g2_3 + g3_4 <= 2;
 + g1_4 + g2_4 + g3_4 <= 2;
 
// do teams 1 and 2 have an opponent in common?
// g1_2 or (g1_3 and g2_3) or (g1_4 and g2_4)
g1_3&2_3 <= g1_3;
g1_3&2_3 <= g2_3;
g1_4&2_4 <= g1_4;
g1_4&2_4 <= g2_4;
g1_2 + g1_3&2_3 + g1_4&2_4 >= 1;
 
// do teams 1 and 3 have an opponent in common?
// g1_3 or (g1_2 and g2_3) or (g1_4 and g3_4)
g1_2&2_3 <= g1_2;
g1_2&2_3 <= g2_3;
g1_4&3_4 <= g1_4;
g1_4&3_4 <= g3_4;
g1_3 + g1_2&2_3 + g1_4&3_4 >= 1;
 
// do teams 1 and 4 have an opponent in common?
// g1_4 or (g1_2 and g2_4) or (g1_3 and g3_4)
g1_2&2_4 <= g1_2;
g1_2&2_4 <= g2_4;
g1_3&3_4 <= g1_3;
g1_3&3_4 <= g3_4;
g1_4 + g1_2&2_4 + g1_3&3_4 >= 1;
 
// do teams 2 and 3 have an opponent in common?
// g2_3 or (g1_2 and g1_3) or (g2_4 and g3_4)
g1_2&1_3 <= g1_2;
g1_2&1_3 <= g1_3;
g2_4&3_4 <= g2_4;
g2_4&3_4 <= g3_4;
g2_3 + g1_2&1_3 + g2_4&3_4 >= 1;
 
// do teams 2 and 4 have an opponent in common?
// g2_4 or (g1_2 and g1_4) or (g2_3 and g3_4)
g1_2&1_4 <= g1_2;
g1_2&1_4 <= g1_4;
g2_3&3_4 <= g2_3;
g2_3&3_4 <= g3_4;
g2_4 + g1_2&1_4 + g2_3&3_4 >= 1;
 
// do teams 3 and 4 have an opponent in common?
// g3_4 or (g1_3 and g1_4) or (g2_3 and g2_4)
g1_3&1_4 <= g1_3;
g1_3&1_4 <= g1_4;
g2_3&2_4 <= g2_3;
g2_3&2_4 <= g2_4;
g3_4 + g1_3&1_4 + g2_3&2_4 >= 1;
 
// binary variables
bin g1_2 g1_3 g1_4 g2_3 g2_4 g3_4 g1_2&1_3 g1_2&1_4 g1_2&2_3 g1_2&2_4 g1_3&1_4 g1_3&2_3 g1_3&3_4 g1_4&2_4 g1_4&3_4 g2_3&2_4 g2_3&3_4 g2_4&3_4 r1_1_2 r2_1_2 r1_1_3 r2_1_3 r1_1_4 r2_1_4 r1_2_3 r2_2_3 r1_2_4 r2_2_4 r1_3_4 r2_3_4;
