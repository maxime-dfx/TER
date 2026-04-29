// Fichier genere automatiquement
SetFactory("OpenCASCADE");

Mesh.SaveAll = 0;
Mesh.CharacteristicLengthMax = 20;
Mesh.CharacteristicLengthMin = 2.5;
Geometry.Tolerance = 1e-6;

Rectangle(1) = {0, 0, 0, 868, 649};

Rectangle(2) = {0, 0, 0, 868, 649};

fibres_finales[] = {};

Disk(1000) = {157, 27, 0, 77.3551};
temp[] = BooleanIntersection{ Surface{1000}; Delete; }{ Surface{2}; };
fibres_finales[] += temp[];

Recursive Delete { Surface{2}; }

// Matrice = domaine - fibres
mat[] = BooleanDifference{ Surface{1}; Delete; }{ Surface{fibres_finales[]}; };
Coherence;

Physical Surface("Matrice", 101) = mat[];
If (#fibres_finales[] > 0)
  Physical Surface("Fibres", 102) = fibres_finales[];
EndIf
