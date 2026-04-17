import os
import sys
import threading
import subprocess
import toml
import customtkinter as ctk
from PIL import Image

# Configuration de l'apparence
ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")

class ApplicationTER(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("CompositeSim v2.0 Professional - Analyse Multi-Échelles")
        self.geometry("1150x850")

        self.toml_path = "etude_master.toml"
        self.grid_columnconfigure(1, weight=1)
        self.grid_rowconfigure(0, weight=1)

        # ==========================================
        # MENU LATÉRAL
        # ==========================================
        self.sidebar = ctk.CTkFrame(self, width=220, corner_radius=0)
        self.sidebar.grid(row=0, column=0, sticky="nsew")
        
        self.logo_label = ctk.CTkLabel(self.sidebar, text="CompositeSim", font=ctk.CTkFont(size=24, weight="bold"))
        self.logo_label.grid(row=0, column=0, padx=20, pady=(20, 30))

        self.btn_parametres = ctk.CTkButton(self.sidebar, text="1. Configuration", command=self.show_config)
        self.btn_parametres.grid(row=1, column=0, padx=20, pady=10)

        self.btn_run = ctk.CTkButton(self.sidebar, text="2. Simulations", command=self.show_run)
        self.btn_run.grid(row=2, column=0, padx=20, pady=10)

        self.btn_visu = ctk.CTkButton(self.sidebar, text="3. Galerie & 3D", command=self.show_visu, fg_color="#E67E22", hover_color="#D35400")
        self.btn_visu.grid(row=3, column=0, padx=20, pady=10)

        # ==========================================
        # FRAME PRINCIPALE : ONGLETS DE CONFIGURATION
        # ==========================================
        self.frame_config = ctk.CTkFrame(self, corner_radius=10)
        self.tabview = ctk.CTkTabview(self.frame_config, width=800)
        self.tabview.pack(padx=20, pady=20, fill="both", expand=True)

        self.tabview.add("Matériaux")
        self.tabview.add("Géométrie & VER")
        
        # --- Onglet Matériaux ---
        self.setup_materiaux_tab()
        
        # --- Onglet Géométrie ---
        self.setup_geometrie_tab()

        self.btn_save = ctk.CTkButton(self.frame_config, text="Appliquer et Sauvegarder TOML", 
                                       command=self.sauvegarder_config, fg_color="#28a745", hover_color="#218838", height=40)
        self.btn_save.pack(pady=(0, 20))

        # ==========================================
        # FRAME : RUN & CONSOLE
        # ==========================================
        self.frame_run = ctk.CTkFrame(self, corner_radius=10)
        ctk.CTkLabel(self.frame_run, text="Lancement des Études", font=ctk.CTkFont(size=22, weight="bold")).pack(pady=10)

        self.interphase_switch = ctk.CTkSwitch(self.frame_run, text="Activer l'Interphase (Matériau ID 3)")
        self.interphase_switch.pack(pady=10)

        frame_btns = ctk.CTkFrame(self.frame_run, fg_color="transparent")
        frame_btns.pack(pady=10, fill="x", padx=50)
        
        self.btn_run_param = ctk.CTkButton(frame_btns, text="Étude Paramétrique (Vf)", command=lambda: self.lancer_tache("src/stats/etude_vf_parametrics.py"))
        self.btn_run_param.pack(side="left", expand=True, padx=5)

        self.btn_run_stats = ctk.CTkButton(frame_btns, text="Stats Méca (Monte Carlo)", command=lambda: self.lancer_tache("src/stats/etude_ver.py"))
        self.btn_run_stats.pack(side="left", expand=True, padx=5)

        self.btn_run_thermo = ctk.CTkButton(frame_btns, text="Stats Thermique", command=lambda: self.lancer_tache("src/stats/etude_thermique_stats.py"))
        self.btn_run_thermo.pack(side="left", expand=True, padx=5)

        self.log_box = ctk.CTkTextbox(self.frame_run, font=ctk.CTkFont(family="Consolas", size=12))
        self.log_box.pack(pady=20, padx=20, fill="both", expand=True)

        # ==========================================
        # FRAME : GALERIE (MIXTE 2D / 3D)
        # ==========================================
        self.frame_visu = ctk.CTkFrame(self, corner_radius=10)
        self.setup_visu_tab()

        # Init
        self.charger_config_actuelle()
        self.show_config()

    def setup_materiaux_tab(self):
        parent = self.tabview.tab("Matériaux")
        cols = ctk.CTkFrame(parent, fg_color="transparent")
        cols.pack(fill="both", expand=True)
        
        # MATRICE
        f_mat = self.create_mat_group(cols, "MATRICE (ID 1)")
        f_mat.grid(row=0, column=0, padx=10, pady=10, sticky="nsew")
        self.ent_em = self.create_field(f_mat, "E (MPa)", "3500.0")
        self.ent_num = self.create_field(f_mat, "nu", "0.35")
        self.ent_am = self.create_field(f_mat, "alpha (1/°C)", "60e-6")
        self.ent_xtm = self.create_field(f_mat, "Xt (MPa)", "80.0")
        self.ent_xcm = self.create_field(f_mat, "Xc (MPa)", "150.0")

        # FIBRE
        f_fib = self.create_mat_group(cols, "FIBRE (ID 2)")
        f_fib.grid(row=0, column=1, padx=10, pady=10, sticky="nsew")
        self.ent_ef = self.create_field(f_fib, "E (MPa)", "280000.0")
        self.ent_nuf = self.create_field(f_fib, "nu", "0.20")
        self.ent_af = self.create_field(f_fib, "alpha (1/°C)", "-1e-6")
        self.ent_xtf = self.create_field(f_fib, "Xt (MPa)", "3000.0")
        self.ent_xcf = self.create_field(f_fib, "Xc (MPa)", "2000.0")

        # INTERPHASE
        f_int = self.create_mat_group(cols, "INTERPHASE (ID 3)")
        f_int.grid(row=0, column=2, padx=10, pady=10, sticky="nsew")
        self.ent_ei_m = self.create_field(f_int, "E (MPa)", "2000.0")
        self.ent_nui = self.create_field(f_int, "nu", "0.38")
        self.ent_ai = self.create_field(f_int, "alpha (1/°C)", "70e-6")
        self.ent_xti = self.create_field(f_int, "Xt (MPa)", "40.0")
        self.ent_xci = self.create_field(f_int, "Xc (MPa)", "100.0")

    def setup_geometrie_tab(self):
        parent = self.tabview.tab("Géométrie & VER")
        self.ent_vf = self.create_field(parent, "Fraction Volumique (Vf)", "0.58")
        self.ent_rf = self.create_field(parent, "Rayon Fibre (µm)", "3.5")
        self.ent_thick = self.create_field(parent, "Épaisseur Interphase (µm)", "0.5")
        self.ent_size = self.create_field(parent, "Taille VER Transverse (µm)", "20.0")
        
        ctk.CTkLabel(parent, text="Paramètres de Calcul Statistiques", font=ctk.CTkFont(size=14, weight="bold")).pack(pady=(30, 10))
        self.ent_samples = self.create_field(parent, "Nb Échantillons (Monte Carlo)", "20")
        self.ent_dt = self.create_field(parent, "Delta T pour Thermique (°C)", "-100.0")

    def setup_visu_tab(self):
        ctk.CTkLabel(self.frame_visu, text="Gestionnaire des Résultats", font=ctk.CTkFont(size=22, weight="bold")).pack(pady=10)
        
        # --- BLOC 3D (PARAVIEW) ---
        f_3d = ctk.CTkFrame(self.frame_visu, fg_color="#2C3E50", corner_radius=8)
        f_3d.pack(fill="x", padx=20, pady=5)
        
        ctk.CTkLabel(f_3d, text="Modèles 3D Éléments Finis (.vtk)", font=ctk.CTkFont(weight="bold")).pack(pady=5)
        f_top_3d = ctk.CTkFrame(f_3d, fg_color="transparent")
        f_top_3d.pack(fill="x", padx=10, pady=5)
        
        self.combo_vtk = ctk.CTkOptionMenu(f_top_3d, values=["Aucun fichier"], width=400)
        self.combo_vtk.pack(side="left", padx=10)
        
        btn_open_vtk = ctk.CTkButton(f_top_3d, text="Lancer le fichier dans ParaView", command=self.ouvrir_vtk_paraview, fg_color="#8E44AD", hover_color="#732D91")
        btn_open_vtk.pack(side="left", padx=10)
        
        ctk.CTkButton(f_top_3d, text="Actualiser", command=self.actualiser_galerie, width=100).pack(side="right", padx=10)

        # --- BLOC 2D (MATPLOTLIB) ---
        f_2d = ctk.CTkFrame(self.frame_visu, fg_color="#273746", corner_radius=8)
        f_2d.pack(fill="x", padx=20, pady=10)
        
        ctk.CTkLabel(f_2d, text="Graphiques Statistiques 2D (.png)", font=ctk.CTkFont(weight="bold")).pack(pady=5)
        f_top_2d = ctk.CTkFrame(f_2d, fg_color="transparent")
        f_top_2d.pack(fill="x", padx=10, pady=5)
        
        self.combo_images = ctk.CTkOptionMenu(f_top_2d, values=["Aucun fichier"], command=self.charger_image_2d, width=400)
        self.combo_images.pack(side="left", padx=10)

        # Affichage de l'image
        self.image_label = ctk.CTkLabel(self.frame_visu, text="Sélectionnez un graphique 2D à afficher")
        self.image_label.pack(pady=10, padx=20, expand=True, fill="both")

    # --- HELPERS UI ---
    def create_mat_group(self, parent, title):
        frame = ctk.CTkFrame(parent)
        ctk.CTkLabel(frame, text=title, font=ctk.CTkFont(weight="bold")).pack(pady=5)
        return frame

    def create_field(self, parent, label_text, default_val):
        f = ctk.CTkFrame(parent, fg_color="transparent")
        f.pack(pady=2, fill="x", padx=20)
        ctk.CTkLabel(f, text=label_text, width=150, anchor="w").pack(side="left")
        e = ctk.CTkEntry(f, width=100)
        e.insert(0, default_val)
        e.pack(side="right")
        return e

    # --- LOGIQUE ---
    def show_config(self): self.cacher_frames(); self.frame_config.grid(row=0, column=1, padx=20, pady=20, sticky="nsew")
    def show_run(self): self.cacher_frames(); self.frame_run.grid(row=0, column=1, padx=20, pady=20, sticky="nsew")
    def show_visu(self): self.cacher_frames(); self.actualiser_galerie(); self.frame_visu.grid(row=0, column=1, padx=20, pady=20, sticky="nsew")
    def cacher_frames(self): self.frame_config.grid_forget(); self.frame_run.grid_forget(); self.frame_visu.grid_forget()

    def charger_config_actuelle(self):
        if not os.path.exists(self.toml_path): return
        try:
            d = toml.load(self.toml_path)
            m = d.get("materiaux", {})
            g = d.get("geometrie", {})
            s = d.get("simulation", {})
            if "matrice" in m: self.ent_em.delete(0, 'end'); self.ent_em.insert(0, m["matrice"].get("E", ""))
            if "fibre" in m: self.ent_ef.delete(0, 'end'); self.ent_ef.insert(0, m["fibre"].get("E", ""))
            if "taille_ver_transverse" in g: self.ent_size.delete(0, 'end'); self.ent_size.insert(0, g["taille_ver_transverse"])
            if "nb_echantillons" in s: self.ent_samples.delete(0, 'end'); self.ent_samples.insert(0, s["nb_echantillons"])
        except: pass

    def sauvegarder_config(self):
        try:
            d = toml.load(self.toml_path) if os.path.exists(self.toml_path) else {}
            d["materiaux"] = {
                "matrice": {"label": 1, "E": float(self.ent_em.get()), "nu": float(self.ent_num.get()), "alpha": float(self.ent_am.get()), "Xt": float(self.ent_xtm.get()), "Xc": float(self.ent_xcm.get())},
                "fibre": {"label": 2, "E": float(self.ent_ef.get()), "nu": float(self.ent_nuf.get()), "alpha": float(self.ent_af.get()), "Xt": float(self.ent_xtf.get()), "Xc": float(self.ent_xcf.get())},
                "interphase": {"label": 3, "E": float(self.ent_ei_m.get()), "nu": float(self.ent_nui.get()), "alpha": float(self.ent_ai.get()), "Xt": float(self.ent_xti.get()), "Xc": float(self.ent_xci.get())}
            }
            d["geometrie"] = {
                "taille_ver_transverse": float(self.ent_size.get()), "rayon_fibre": float(self.ent_rf.get()), 
                "epaisseur_interphase": float(self.ent_thick.get()), "fraction_volumique": float(self.ent_vf.get())
            }
            d["simulation"] = { "nb_echantillons": int(self.ent_samples.get()), "delta_T": float(self.ent_dt.get()) }
            with open(self.toml_path, "w") as f: toml.dump(d, f)
            self.log_safe("> [TOML] Configuration sauvegardée.")
        except Exception as e: self.log_safe(f"> [ERREUR] {e}")

    def lancer_tache(self, script_path):
        self.sauvegarder_config()
        threading.Thread(target=self.executer_processus, args=(script_path,), daemon=True).start()

    def executer_processus(self, path):
        cmd = [sys.executable, path]
        if self.interphase_switch.get(): cmd.append("--interphase")
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        for line in iter(p.stdout.readline, ''): self.after(0, self.log_safe, line.strip())
        p.wait()

    # --- NOUVEAU GESTIONNAIRE MIXTE 2D / 3D ---
    def actualiser_galerie(self):
        # 1. Gestion des PNG (2D)
        if os.path.exists("results"):
            f_png = [f for f in os.listdir("results") if f.endswith(".png")]
            if f_png:
                f_png.sort(key=lambda x: os.path.getmtime(os.path.join("results", x)), reverse=True)
                self.combo_images.configure(values=f_png)
                self.combo_images.set(f_png[0])
                self.charger_image_2d(f_png[0])
            else:
                self.combo_images.configure(values=["Aucun graphique généré"])
                self.combo_images.set("Aucun graphique généré")

        # 2. Gestion des VTK (3D)
        vtk_files = []
        if os.path.exists("workspace"):
            for root, dirs, files in os.walk("workspace"):
                for f in files:
                    if f.endswith(".vtk"):
                        # Stocke le chemin relatif pour un affichage propre (ex: Fibre_Carbone\res_tx.vtk)
                        chemin_relatif = os.path.relpath(os.path.join(root, f), "workspace")
                        vtk_files.append(chemin_relatif)
        
        if vtk_files:
            self.combo_vtk.configure(values=vtk_files)
            self.combo_vtk.set(vtk_files[-1]) # Sélectionne le plus récent (en général le dernier de la liste)
        else:
            self.combo_vtk.configure(values=["Aucun modèle 3D généré"])
            self.combo_vtk.set("Aucun modèle 3D généré")

    def charger_image_2d(self, name):
        p = os.path.join("results", name)
        if not os.path.exists(p): return
        img = Image.open(p)
        mw, mh = 850, 500
        r_img, r_box = img.width/img.height, mw/mh
        if r_img > r_box: w, h = mw, int(mw/r_img)
        else: w, h = int(mh*r_img), mh
        self.image_label.configure(image=ctk.CTkImage(img, img, size=(w,h)), text="")

    def ouvrir_vtk_paraview(self):
        selection = self.combo_vtk.get()
        if "Aucun" in selection: return
        
        chemin_absolu = os.path.abspath(os.path.join("workspace", selection))
        try:
            # os.startfile utilise l'association de fichiers de Windows.
            # Si ParaView est associé aux fichiers .vtk, il s'ouvrira tout seul !
            os.startfile(chemin_absolu)
            self.log_safe(f"\n> Ouverture de ParaView pour : {selection}")
        except Exception as e:
            self.log_safe(f"\n> [ERREUR] Impossible d'ouvrir le fichier. Vérifiez que ParaView est installé. {e}")

    def log_safe(self, t): self.log_box.insert("end", t+"\n"); self.log_box.see("end")

if __name__ == "__main__": ApplicationTER().mainloop()