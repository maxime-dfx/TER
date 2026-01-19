// --- test_vide.geo : Matrice Seule ---
SetFactory("OpenCASCADE");

// Paramètres
W = 868;
H = 649;
lc = 20; // Maillage assez grossier pour aller vite

// 1. Géométrie : Juste un rectangle
Rectangle(1) = {0, 0, 0, W, H};

// 2. Physical Groups
// IMPORTANT : On garde le tag 101 que votre solveur attend pour la matrice
Physical Surface("Matrice", 101) = {1};

// Note : Pas de tag 102 (Fibres) ici.
// Votre MaterialManager a le matériau 102 défini, mais comme aucun élément
// du maillage ne portera ce label, il ne sera simplement jamais utilisé.

// 3. Maillage
Mesh.CharacteristicLengthMin = lc;
Mesh.CharacteristicLengthMax = lc;