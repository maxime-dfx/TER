# src/physics/geometry_utils.py
import os
import csv
import math
import random
from PIL import Image

def read_image_and_csv(csv_path, img_path, L):
    """
    Lit les clics bruts et les convertit en coordonnées physiques (micromètres).
    Retourne: (positions, rayons)
    """
    if not os.path.exists(csv_path):
        return [], []
        
    # 1. Lecture du CSV
    raw_pos, raw_rad = [], []
    with open(csv_path, 'r') as f:
        reader = csv.reader(f)
        for row in reader:
            if len(row) >= 3:
                raw_pos.append((float(row[0]), float(row[1])))
                rx = float(row[2])
                ry = float(row[3]) if len(row) > 3 else rx
                raw_rad.append((rx, ry))
                
    if not raw_pos: return [], []

    # 2. Lecture Image et Scaling
    W, H = 1000, 1000
    if os.path.exists(img_path):
        with Image.open(img_path) as img:
            W, H = img.size

    scale_css = L / max(W, H)
    
    # 3. Conversion
    positions, rayons = [], []
    for p, r in zip(raw_pos, raw_rad):
        px_micron = (p[0] - W / 2.0) * scale_css
        py_micron = -(p[1] - H / 2.0) * scale_css 
        positions.append((px_micron, py_micron))
        rayons.append((r[0] * scale_css, r[1] * scale_css))
        
    return positions, rayons

def generer_positions_aleatoires(L, R_total, Vf):
    """
    Génère un nuage de points aléatoires sans collision pour atteindre 
    la fraction volumique (Vf) demandée.
    """
    positions = []
    surface_totale = L * L
    surface_fibre = math.pi * (R_total**2)
    nb_fibres_cible = int((Vf * surface_totale) / surface_fibre)
    
    attempts = 0
    while len(positions) < nb_fibres_cible and attempts < 10000:
        x = random.uniform(-L/2 + R_total, L/2 - R_total)
        y = random.uniform(-L/2 + R_total, L/2 - R_total)
        collision = False
        
        for (px, py) in positions:
            # On vérifie qu'il n'y a pas d'intersection entre les fibres
            if math.hypot(x - px, y - py) < (2 * R_total + 0.1):
                collision = True
                break
                
        if not collision: 
            positions.append((x, y))
        attempts += 1
        
    print(f"      [+] Génération aléatoire : {len(positions)} fibres placées (Vf visé: {Vf}).")
    return positions