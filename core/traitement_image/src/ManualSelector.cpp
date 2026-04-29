#define _USE_MATH_DEFINES
#include "ManualSelector.h"
#include <iostream>
#include <cmath>

using namespace cv;
using namespace std;

ManualSelector::ManualSelector(const string& imagePath)
    : m_windowName("Selection Manuelle"),
      m_mode(SelectionMode::CIRCLE),
      m_step1Done(false),
      m_step2Done(false)
{
    m_imgRaw = imread(imagePath);
    if (m_imgRaw.empty()) {
        cerr << "Erreur: Impossible de charger " << imagePath << endl;
        exit(-1);
    }
    m_imgDisplay = m_imgRaw.clone();
}

void ManualSelector::mouseCallback(int event, int x, int y, int /*flags*/, void* userdata) {
    static_cast<ManualSelector*>(userdata)->handleMouse(event, x, y);
}

void ManualSelector::handleMouse(int event, int x, int y) {

    // ---- CLIC GAUCHE ----
    if (event == EVENT_LBUTTONDOWN) {

        // --- CERCLE : 2 clics ---
        if (m_mode == SelectionMode::CIRCLE) {
            if (!m_step1Done) {
                m_tempCenter = Point2f((float)x, (float)y);
                m_step1Done = true;
                cout << "Cercle - centre (" << x << "," << y << ") -> cliquez le bord." << endl;
            } else {
                double r = norm(Point2f((float)x, (float)y) - m_tempCenter);
                m_fibres.push_back(Fibre(m_tempCenter, r));
                m_step1Done = false;
                cout << "Cercle ajoute R=" << (int)r << "  | total=" << m_fibres.size() << endl;
                updateDisplay();
            }
        }

        // --- ELLIPSE : 3 clics ---
        else if (m_mode == SelectionMode::ELLIPSE) {
            if (!m_step1Done) {
                m_tempCenter = Point2f((float)x, (float)y);
                m_step1Done = true;
                cout << "Ellipse - centre -> cliquez bout du grand axe." << endl;
            } else if (!m_step2Done) {
                m_tempAxisEnd = Point2f((float)x, (float)y);
                m_step2Done = true;
                double rx = norm(m_tempAxisEnd - m_tempCenter);
                cout << "Ellipse - grand axe=" << (int)rx << "px -> cliquez bout du petit axe." << endl;
            } else {
                double dx = m_tempAxisEnd.x - m_tempCenter.x;
                double dy = m_tempAxisEnd.y - m_tempCenter.y;
                double rx = sqrt(dx*dx + dy*dy);
                double angle = atan2(dy, dx);
                double pdx = x - m_tempCenter.x;
                double pdy = y - m_tempCenter.y;
                double ry = max(1.0, abs(-pdx * sin(angle) + pdy * cos(angle)));
                m_fibres.push_back(Fibre(m_tempCenter, rx, ry, angle));
                m_step1Done = m_step2Done = false;
                cout << "Ellipse ajoutee rx=" << (int)rx << " ry=" << (int)ry
                     << " angle=" << (int)(angle*180.0/M_PI) << "deg"
                     << "  | total=" << m_fibres.size() << endl;
                updateDisplay();
            }
        }

        // --- RECTANGLE : 3 clics (meme logique qu'ellipse) ---
        else if (m_mode == SelectionMode::RECTANGLE) {
            if (!m_step1Done) {
                m_tempCenter = Point2f((float)x, (float)y);
                m_step1Done = true;
                cout << "Rectangle - centre -> cliquez bout du grand cote." << endl;
            } else if (!m_step2Done) {
                m_tempAxisEnd = Point2f((float)x, (float)y);
                m_step2Done = true;
                double halfLen = norm(m_tempAxisEnd - m_tempCenter);
                cout << "Rectangle - demi-longueur=" << (int)halfLen << "px -> cliquez le bord court." << endl;
            } else {
                double dx = m_tempAxisEnd.x - m_tempCenter.x;
                double dy = m_tempAxisEnd.y - m_tempCenter.y;
                double halfLen = sqrt(dx*dx + dy*dy);
                double angle = atan2(dy, dx);
                double pdx = x - m_tempCenter.x;
                double pdy = y - m_tempCenter.y;
                double halfWidth = max(1.0, abs(-pdx * sin(angle) + pdy * cos(angle)));
                m_fibres.push_back(Fibre::makeRect(m_tempCenter, halfLen, halfWidth, angle));
                m_step1Done = m_step2Done = false;
                cout << "Rectangle ajoute L=" << (int)(2*halfLen) << " l=" << (int)(2*halfWidth)
                     << " angle=" << (int)(angle*180.0/M_PI) << "deg"
                     << "  | total=" << m_fibres.size() << endl;
                updateDisplay();
            }
        }
    }

    // ---- CLIC DROIT : annuler ----
    if (event == EVENT_RBUTTONDOWN) {
        if (m_step2Done) {
            m_step2Done = false;
            cout << "Etape 3 annulee, recommencez." << endl;
        } else if (m_step1Done) {
            m_step1Done = false;
            cout << "Saisie annulee." << endl;
        } else if (!m_fibres.empty()) {
            m_fibres.pop_back();
            cout << "Derniere fibre supprimee. total=" << m_fibres.size() << endl;
        }
        updateDisplay();
    }

    // ---- DEPLACEMENT : previsualisation ----
    if (event == EVENT_MOUSEMOVE) {
        Mat temp = m_imgDisplay.clone();

        if (m_mode == SelectionMode::CIRCLE && m_step1Done) {
            double r = norm(Point2f((float)x,(float)y) - m_tempCenter);
            circle(temp, m_tempCenter, (int)r, Scalar(0,165,255), 1);
            circle(temp, m_tempCenter, 3, Scalar(0,0,255), -1);
        }
        else if ((m_mode == SelectionMode::ELLIPSE || m_mode == SelectionMode::RECTANGLE)
                 && m_step1Done && !m_step2Done) {
            line(temp, m_tempCenter, Point(x,y), Scalar(0,165,255), 1);
            circle(temp, m_tempCenter, 3, Scalar(0,0,255), -1);
        }
        else if ((m_mode == SelectionMode::ELLIPSE || m_mode == SelectionMode::RECTANGLE)
                 && m_step1Done && m_step2Done) {
            double dx = m_tempAxisEnd.x - m_tempCenter.x;
            double dy = m_tempAxisEnd.y - m_tempCenter.y;
            double rx = sqrt(dx*dx + dy*dy);
            double angle = atan2(dy, dx);
            double pdx = x - m_tempCenter.x;
            double pdy = y - m_tempCenter.y;
            double ry = max(1.0, abs(-pdx*sin(angle) + pdy*cos(angle)));
            int angleDeg = (int)(angle * 180.0 / M_PI);

            if (m_mode == SelectionMode::ELLIPSE) {
                ellipse(temp, m_tempCenter, Size((int)rx,(int)ry), angleDeg,
                        0, 360, Scalar(0,165,255), 1);
            } else {
                // Previsualisation rectangle
                Point2f pts[4];
                double ca = cos(angle), sa = sin(angle);
                pts[0] = m_tempCenter + Point2f((float)(rx*ca - ry*sa), (float)(rx*sa + ry*ca));
                pts[1] = m_tempCenter + Point2f((float)(-rx*ca - ry*sa), (float)(-rx*sa + ry*ca));
                pts[2] = m_tempCenter + Point2f((float)(-rx*ca + ry*sa), (float)(-rx*sa - ry*ca));
                pts[3] = m_tempCenter + Point2f((float)(rx*ca + ry*sa), (float)(rx*sa - ry*ca));
                for (int i = 0; i < 4; ++i)
                    line(temp, pts[i], pts[(i+1)%4], Scalar(0,165,255), 1);
            }
            circle(temp, m_tempCenter, 3, Scalar(0,0,255), -1);
            line(temp, m_tempCenter, Point(x,y), Scalar(100,100,255), 1);
        }

        imshow(m_windowName, temp);
        return;
    }

    // Rafraichissement statique apres un clic
    if (event == EVENT_LBUTTONDOWN || event == EVENT_RBUTTONDOWN) {
        updateDisplay();
    }
}

void ManualSelector::updateDisplay() {
    m_imgDisplay = m_imgRaw.clone();

    // Dessiner toutes les fibres validees
    for (const auto& f : m_fibres) {
        int angleDeg = (int)(f.angle * 180.0 / CV_PI);
        if (f.shape == FibreShape::CIRCLE) {
            circle(m_imgDisplay, f.center, (int)f.radius, Scalar(0,255,0), 2);
        }
        else if (f.shape == FibreShape::ELLIPSE) {
            ellipse(m_imgDisplay, f.center, Size((int)f.rx,(int)f.ry),
                    angleDeg, 0, 360, Scalar(0,200,255), 2);
        }
        else if (f.shape == FibreShape::RECTANGLE) {
            double ca = cos(f.angle), sa = sin(f.angle);
            Point2f pts[4];
            pts[0] = f.center + Point2f((float)( f.rx*ca - f.ry*sa), (float)( f.rx*sa + f.ry*ca));
            pts[1] = f.center + Point2f((float)(-f.rx*ca - f.ry*sa), (float)(-f.rx*sa + f.ry*ca));
            pts[2] = f.center + Point2f((float)(-f.rx*ca + f.ry*sa), (float)(-f.rx*sa - f.ry*ca));
            pts[3] = f.center + Point2f((float)( f.rx*ca + f.ry*sa), (float)( f.rx*sa - f.ry*ca));
            for (int i = 0; i < 4; ++i)
                line(m_imgDisplay, pts[i], pts[(i+1)%4], Scalar(255,140,0), 2);
        }
        circle(m_imgDisplay, f.center, 3, Scalar(0,0,255), -1);
    }

    // Indicateur de mode
    string modeStr;
    Scalar modeColor;
    if      (m_mode == SelectionMode::CIRCLE)    { modeStr = "Mode: CERCLE    [E]=ellipse [R]=rect"; modeColor = Scalar(0,255,0); }
    else if (m_mode == SelectionMode::ELLIPSE)   { modeStr = "Mode: ELLIPSE   [C]=cercle  [R]=rect"; modeColor = Scalar(0,200,255); }
    else                                          { modeStr = "Mode: RECTANGLE [C]=cercle  [E]=ellipse"; modeColor = Scalar(255,140,0); }
    putText(m_imgDisplay, modeStr, Point(10,25), FONT_HERSHEY_SIMPLEX, 0.65, modeColor, 2);

    // Etape en cours
    string step = "";
    if (m_step1Done && !m_step2Done) {
        if (m_mode == SelectionMode::CIRCLE) step = "-> cliquez le bord";
        else                                  step = "-> cliquez bout du grand axe";
    } else if (m_step2Done) {
        step = "-> cliquez le bord court / petit axe";
    }
    if (!step.empty())
        putText(m_imgDisplay, step, Point(10,52), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255,200,0), 2);

    // Compteur
    putText(m_imgDisplay, "Fibres: " + to_string(m_fibres.size()),
            Point(10, m_imgDisplay.rows - 10), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255,255,255), 2);

    imshow(m_windowName, m_imgDisplay);
}

void ManualSelector::printHelp() {
    cout << "\n=== MODE SELECTION MANUELLE ===" << endl;
    cout << "  [C] Mode cercle    : centre + bord (2 clics)" << endl;
    cout << "  [E] Mode ellipse   : centre + grand axe + petit axe (3 clics)" << endl;
    cout << "  [R] Mode rectangle : centre + grand cote + petit cote (3 clics)" << endl;
    cout << "  Clic droit : annuler etape / supprimer derniere fibre" << endl;
    cout << "  [ESPACE]   : terminer et generer le maillage" << endl;
    cout << "  [ESC]      : quitter" << endl;
    cout << "================================\n" << endl;
}

vector<Fibre> ManualSelector::run() {
    namedWindow(m_windowName, WINDOW_AUTOSIZE);
    setMouseCallback(m_windowName, mouseCallback, this);
    printHelp();
    updateDisplay();

    while (true) {
        int k = waitKey(10);
        if (k == ' ' || k == 27) break;
        if (k == 'c' || k == 'C') {
            m_mode = SelectionMode::CIRCLE;
            m_step1Done = m_step2Done = false;
            cout << "Mode CERCLE." << endl; updateDisplay();
        }
        if (k == 'e' || k == 'E') {
            m_mode = SelectionMode::ELLIPSE;
            m_step1Done = m_step2Done = false;
            cout << "Mode ELLIPSE." << endl; updateDisplay();
        }
        if (k == 'r' || k == 'R') {
            m_mode = SelectionMode::RECTANGLE;
            m_step1Done = m_step2Done = false;
            cout << "Mode RECTANGLE." << endl; updateDisplay();
        }
        if (k == 127 || k == 8) {  // Suppr ou Backspace
            if (!m_fibres.empty()) {
                m_fibres.pop_back();
                cout << "Supprime. total=" << m_fibres.size() << endl;
                updateDisplay();
            }
        }
    }
    destroyWindow(m_windowName);
    return m_fibres;
}
