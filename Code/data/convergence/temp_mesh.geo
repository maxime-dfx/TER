SetFactory("OpenCASCADE");
lc = 2;
L = 100; R = 30;
Rectangle(1) = {0, 0, 0, L, L};
Disk(2) = {L/2, L/2, 0, R};
v[] = BooleanFragments{ Surface{1}; Delete; }{ Surface{2}; Delete; };
surf_mat[] = {};
surf_fib[] = {};
eps = 1e-3;
For k In {0 : #v[]-1}
  bb[] = BoundingBox Surface{ v[k] };
  width = bb[3] - bb[0];
  If ( width > L - 1 )
    surf_mat[] += v[k];
  Else
    surf_fib[] += v[k];
  EndIf
EndFor
Physical Surface("Matrice", 101) = surf_mat[];
Physical Surface("Fibre", 102) = surf_fib[];
Mesh.CharacteristicLengthMin = lc;
Mesh.CharacteristicLengthMax = lc;
