// --- Fichier généré : Matrice + Fibres (Tri Automatique) ---
SetFactory("OpenCASCADE");

lc_mat = 20;
lc_fib = 5;

Geometry.Tolerance = 1e-6;
Mesh.CharacteristicLengthMin = 0;

Rectangle(1) = {0, 0, 0, 868, 649};
Rectangle(2) = {0, 0, 0, 868, 649};

fibres_finales[] = {};

Disk(1000) = {290, 528, 0, 133.6};
temp_cut[] = BooleanIntersection{ Surface{1000}; Delete; }{ Surface{2}; };
fibres_finales[] += temp_cut[];
Disk(1001) = {472, 292, 0, 88.6454};
temp_cut[] = BooleanIntersection{ Surface{1001}; Delete; }{ Surface{2}; };
fibres_finales[] += temp_cut[];
Disk(1002) = {727, 566, 0, 125.363};
temp_cut[] = BooleanIntersection{ Surface{1002}; Delete; }{ Surface{2}; };
fibres_finales[] += temp_cut[];

Recursive Delete { Surface{2}; }

// Fusion Finale
parts[] = BooleanFragments{ Surface{1}; Delete; }{ Surface{fibres_finales[]}; Delete; };

Coherence; RemoveAllDuplicates;

// --- Identification Automatique (Matrice vs Fibres) ---
surf_matrice[] = {};
surf_fibres[] = {};

eps = 1e-3; // Tolérance
W_tot = 868;
H_tot = 649;

For k In {0 : #parts[]-1}
  // On récupère la boite englobante de la surface k
  bb[] = BoundingBox Surface{ parts[k] };
  xmin = bb[0]; ymin = bb[1];
  xmax = bb[3]; ymax = bb[4];

  // Si la surface fait la taille de l'image, c'est la Matrice
  If ( Abs(xmax - xmin - W_tot) < eps && Abs(ymax - ymin - H_tot) < eps )
    surf_matrice[] += parts[k];
  Else
    surf_fibres[] += parts[k];
  EndIf
EndFor

// Maillage Adaptatif
MeshSize{ PointsOf{ Surface{surf_matrice[]} } } = lc_mat;
MeshSize{ PointsOf{ Surface{surf_fibres[]} } } = lc_fib;

// Physical Groups
Physical Surface("Matrice", 101) = surf_matrice[];
If (#surf_fibres[] > 0)
  Physical Surface("Fibres", 102) = surf_fibres[];
  Color Red { Surface{ surf_fibres[] }; }
EndIf
Color Blue { Surface{ surf_matrice[] }; }
