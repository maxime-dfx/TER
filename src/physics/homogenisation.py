import numpy as np

def calculer_matrice_rigidite(E1, E2, nu12, nu23, G12):
    """
    Calcule la matrice de rigidité 6x6 d'un matériau composite isotrope transverse.
    Convention : L'axe 1 (Z) est l'axe des fibres. Le plan 2-3 (X-Y) est le plan d'isotropie.
    """
    if None in (E1, E2, nu12, nu23, G12):
        print("\n      [!] Impossible de synthétiser le tenseur : calculs FEA incomplets.")
        return None

    # Hypothèses d'isotropie transverse
    E3 = E2
    nu13 = nu12
    G13 = G12
    
    # Calcul de G23 via la relation d'isotropie dans le plan transverse
    G23 = E2 / (2.0 * (1.0 + nu23))
    
    # Relations de symétrie thermodynamique (Matrice de souplesse symétrique)
    nu21 = nu12 * (E2 / E1)
    nu31 = nu13 * (E3 / E1)
    nu32 = nu23 * (E3 / E2) # Toujours égal à nu23 car E3 = E2

    # Remplissage de la matrice de souplesse S (Compliance matrix)
    S = np.zeros((6, 6))
    
    S[0, 0] = 1.0 / E1
    S[1, 1] = 1.0 / E2
    S[2, 2] = 1.0 / E3
    
    S[0, 1] = S[1, 0] = -nu21 / E2  # équivalent à -nu12 / E1
    S[0, 2] = S[2, 0] = -nu31 / E3
    S[1, 2] = S[2, 1] = -nu32 / E3
    
    S[3, 3] = 1.0 / G23
    S[4, 4] = 1.0 / G13
    S[5, 5] = 1.0 / G12

    try:
        # Inversion du tenseur pour obtenir la matrice de rigidité C
        C = np.linalg.inv(S)
        
        # Nettoyage des zéros absolus pour un affichage propre
        C[np.abs(C) < 1e-10] = 0.0
        return C
    except np.linalg.LinAlgError:
        print("\n      [!] Erreur Mathématique : La matrice de souplesse est singulière.")
        return None

def formater_resultats(C_eff):
    """Affiche le tenseur de rigidité dans la console de manière propre."""
    print("\n      =======================================================")
    print("      TENSEUR DE RIGIDITÉ MACROSCOPIQUE C_eff (en GPa)")
    print("      =======================================================")
    for i in range(6):
        row = "      [ " + "  ".join([f"{val:7.2f}" for val in C_eff[i]]) + " ]"
        print(row)
    print("      =======================================================")