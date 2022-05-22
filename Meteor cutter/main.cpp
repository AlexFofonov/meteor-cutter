#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <ctime>
#include <random>
#include <string>
using namespace std;

const double xmin = -10, xmax = 10, ymin = -10, ymax = 10;
const int width = 700, height = 700;
const double EPS = 0.00000001;
const int timerFrequency = 2; //увеличить для замедления геймплея
const int MAX_SCORE = 3;

bool isStart = false;
bool isFirstCutterFixed = false;
bool isSecondCutterFixed = false;

bool isShellCut = false;
bool isCoreCut = false;

int playerColor = 0; //четное -> ход красного; нечетное -> ход синего
int meteorColor;
double sliderStep = -9; //ход слайдера по [-9;9] по OX
char sliderCycleSign = 1;
double cycleVariable = 0;

int blueScore = 0, redScore = 0;

void reshape(int width, int height)
{
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(xmin, xmax, ymin, ymax);
}

class Point
{
public:
    double x;
    double y;

    Point()
    {
        x = 0;
        y = 0;
    }

    Point(double x_, double y_) : x(x_), y(y_) {}
};

//векотры точек для оболочки метеорита, ядра, пореза
vector <Point> shell, core, cutter;

//вычисление псевдокалярного (косого) произведения
double skewProduct(Point A, Point A1, Point B, Point B1)
{
    return ((A1.x - A.x) * (B1.y - B.y) - (A1.y - A.y) * (B1.x - B.x));
}

//для отрисовки надписей
void renderBitmapString(float x, float y, void *font, string str)
{
    glColor3ub(250, 250, 250);
    glRasterPos2f(x,y);
    for (string::iterator c = (&str)->begin(); c != (&str)->end(); ++c)
    {
        glutBitmapCharacter(font, *c);
    }
}

void drawPoint(Point& argA)
{
    glPointSize(14);
    glBegin(GL_POINTS);
    glColor3f(0, 1, 0);
    glVertex2f(argA.x, argA.y);
    glEnd();
}

void drawCut(Point& argA, Point& argB, double shift, int lineWidth)
{
    glLineWidth(lineWidth);
    glBegin(GL_LINES);
    glColor4f(1 - shift, 1, 1 - shift, shift);
    glVertex2f(argA.x, argA.y);
    glVertex2f(argB.x, argB.y);
    glEnd();
}

void showBackground()
{
    glLineWidth(1);
    glBegin(GL_LINES);
    for (int i=xmin; i<xmax; i++)
    {
        glColor3ub(150, 100, 100);
        glVertex2f(i, ymin);
        glVertex2f(xmin - i, ymax);
        glVertex2f(xmin, i);
        glVertex2f(xmax, ymax - i);
        glColor3ub(100, 150, 100);
        glVertex2f(i, ymin);
        glVertex2f(xmax- i, ymax);
        glVertex2f(xmin - 3, i);
        glVertex2f(xmax, ymax - i - 7);
    }
    glEnd();
}

void showMainScreen()
{
    string temp = "METE0R CUTTER";
    renderBitmapString(-2.5, 0, GLUT_BITMAP_HELVETICA_18, temp);

    temp = "(press right mouse button to start)";
    renderBitmapString(-3, -0.7, GLUT_BITMAP_HELVETICA_12, temp);

    temp = "two players edition";
    renderBitmapString(-1.9, -9.5, GLUT_BITMAP_HELVETICA_12, temp);

    //обозначение победителя полосой его цвета
    if (redScore == MAX_SCORE)
    {
        glColor4f(0.8, 0.2, 0.2, 0.8);
        glBegin(GL_POLYGON);
        glVertex2f(-9.7, 10);
        glVertex2f(-9, 10);
        glVertex2f(-9, -10);
        glVertex2f(-9.7, -10);
        glEnd();
    }

    if (blueScore == MAX_SCORE)
    {
        glColor4f(0.2, 0.2, 0.8, 0.8);
        glBegin(GL_POLYGON);
        glVertex2f(9.7, 10);
        glVertex2f(9, 10);
        glVertex2f(9, -10);
        glVertex2f(9.7, -10);
        glEnd();
    }
}

void showScore()
{
    glColor4f(0.8, 0.2, 0.2, 0.8);
    glBegin(GL_POLYGON);
    glVertex2f(-9.7, 1);
    glVertex2f(-9, 1);
    glVertex2f(-9, -1);
    glVertex2f(-9.7, -1);
    glEnd();

    glColor4f(0.2, 0.2, 0.8, 0.8);
    glBegin(GL_POLYGON);
    glVertex2f(9.7, 1);
    glVertex2f(9, 1);
    glVertex2f(9, -1);
    glVertex2f(9.7, -1);
    glEnd();

    renderBitmapString(-9.5, -0.2, GLUT_BITMAP_TIMES_ROMAN_24, to_string(redScore));
    renderBitmapString(9.15, -0.2, GLUT_BITMAP_TIMES_ROMAN_24, to_string(blueScore));
}

void showGameScreen()
{
    //отрисовка нижнего и верхнего слайдера
    (playerColor % 2 == 0) ? glColor4f(0.8, 0.2, 0.2, 0.8) : glColor4f(0.2, 0.2, 0.8, 0.8);
    glBegin(GL_POLYGON);
    glVertex2f(-9.5, -8.7);
    glVertex2f(9.5, -8.7);
    glVertex2f(9.5, -9);
    glVertex2f(-9.5, -9);
    glEnd();
    glBegin(GL_POLYGON);
    glVertex2f(-9.5, 8.7);
    glVertex2f(9.5, 8.7);
    glVertex2f(9.5, 9);
    glVertex2f(-9.5, 9);
    glEnd();

    //отрисовка тени метеорита (путем смещения координат оболочки)
    glColor4f(0, 0, 0, 0.7);
    glBegin(GL_POLYGON);
    for (double i = 0; i < shell.size() - 1; i++)
    {
        glVertex2f(shell[i].x - 0.5, shell[i].y - 0.5);
    }
    glEnd();

    //отрисовка метеорита
    glLineWidth(7);
    switch (meteorColor)
    {
    case 1:
        glColor3f(0.3, 0.7, 1);
        break;
    case 2:
        glColor3f(0.3, 1, 0.3);
        break;
    case 3:
        glColor3f(1, 0.7, 0.3);
        break;
    case 4:
        glColor3f(1, 0.3, 0.7);
        break;
    case 5:
        glColor3f(0.7, 0.3, 1);
        break;
    case 6:
        glColor3f(0.7, 1, 0.3);
        break;
    case 7:
        glColor3f(0.5, 0.5, 0.5);
        break;
    }

    glBegin(GL_POLYGON);
    for (double i = 0; i < shell.size() - 1; i++)
    {
        glVertex2f(shell[i].x, shell[i].y);
    }
    glEnd();

    //отрисовка ядер
    glColor3f(1, 1, 1);
    glBegin(GL_POLYGON);
    for (double i = 0; i < core.size() - 1; i++)
    {
        glVertex2f(core[i].x, core[i].y);
    }
    glEnd();

    //отрисовка точек резчика
    if (!isFirstCutterFixed && !isSecondCutterFixed)
        drawPoint(cutter[0]);

    if (isFirstCutterFixed)
        drawPoint(cutter[0]);

    if (isSecondCutterFixed)
        drawPoint(cutter[1]);

    if (isFirstCutterFixed && !isSecondCutterFixed)
        drawPoint(cutter[1]);
}

void display(void)
{
    glClearColor(0.0, 0.0, 0.1, 0.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3ub(150, 150, 150);

    showBackground();

    (isStart) ? showGameScreen() : showMainScreen();

    showScore();

    glutSwapBuffers();
}

void makeMeteor()
{
    shell.clear();
    core.clear();

    //рандомим цвет метеорита пока он не будет отличным от прошлого
    int localRand = rand() % 7 + 1;
    while (meteorColor == localRand)
    {
        localRand = rand() % 7 + 1;
    }
    meteorColor = localRand;

    //создание оболочки метеорита (создание случайного выпуклого полигон)
    double xTemp, yTemp, corner = (double) (rand() % 5 + 1) / 10;
    Point support, temp;

    xTemp = (rand() % 2 - 1);
    yTemp = (rand() % 2 - 3);
    support = Point(xTemp,yTemp);

    shell.push_back(Point(xTemp, yTemp));

    while (true)
    {
        xTemp = shell[shell.size() - 1].x + 4.5 * cos(corner);
        yTemp = shell[shell.size() - 1].y + 2.5 * tan(corner) * cos(corner);
        temp = Point(xTemp,yTemp);

        shell.push_back(temp);

        corner += ((rand() % 10 + 4) / (double) 10);

        if ((skewProduct(shell[shell.size() - 2], shell[shell.size() - 1], shell[shell.size() - 2], shell[0]) < EPS
                || skewProduct(shell[0], shell[1], shell[0], shell[shell.size() - 1]) < EPS) && shell.size() > 2)
        {
            shell.pop_back();
            break;
        }
    }

    shell.push_back(shell[0]);

    //ищем середину оболочки -> опорную точку ядра
    core.push_back(Point(0, 0));

    for (double i = 0; i < shell.size() - 1; i++)
    {
        core[0].x += shell[i].x;
        core[0].y += shell[i].y;
    }

    core[0].x /= (shell.size() - 1);
    core[0].y /= (shell.size() - 1);

    //создание ядра (выпуклый полигон внутри метеорита)
    double mainCorner = (double)(rand() % 10 + 5)/10;
    corner = 0;

    while (true)
    {
        xTemp = core[core.size() - 1].x + 0.5 * cos(corner);
        yTemp = core[core.size() - 1].y + 0.5 * tan(corner) * cos(corner);
        temp = Point(xTemp,yTemp);

        core.push_back(temp);

        corner += mainCorner;

        if ((skewProduct(core[core.size() - 2], core[core.size() - 1], core[core.size() - 2], core[0]) < EPS
                || skewProduct(core[0], core[1], core[0], core[core.size() - 1]) < EPS) && core.size() > 2)
        {
            core.pop_back();
            break;
        }
    }

    core.push_back(core[0]);
}

//если все повороты осуществяляются в одну сторону -> пореза нет
bool isCut(vector<Point> v)
{
    int negative = 0, positive = 0;
    double temp;

    for (double i = 0; i < v.size() - 1; i++)
    {
        temp = skewProduct(cutter[0], cutter[1], cutter[0], v[i]);

        if (temp < EPS && temp > 0)
            continue;
        else
            (temp > 0) ? positive++ : negative++;
    }

    if (negative == 0 || positive == 0)
        return false;
    else
        return true;
}

void cutMeteor()
{
    //знак поворота от резчика к ядру
    char coreRotationSign = 1;

    if (skewProduct(cutter[0], cutter[1], cutter[0], core[0]) < 0)
        coreRotationSign = -1;

    //iCut = индекс первой точки из отсекаемых, jCut = индекс точки, на которой кончаются точки для отсечения
    double iCut, jCut;
    for (double i = 0; i < shell.size() - 1; i++)
    {
        if (skewProduct(cutter[0], cutter[1], cutter[0], shell[i]) < 0
            && skewProduct(cutter[0], cutter[1], cutter[0], shell[i+1]) > 0)
        {
            (coreRotationSign < 0) ? iCut = i + 1 : jCut = i + 1;
        }
        if (skewProduct(cutter[0], cutter[1], cutter[0], shell[i]) > 0
            && skewProduct(cutter[0], cutter[1], cutter[0], shell[i+1]) < 0)
        {
            (coreRotationSign > 0) ? iCut = i + 1 : jCut = i + 1;
        }
    }

    Point jCutPoint(shell[jCut].x, shell[jCut].y);

    //находим точки пересечения оболочки с надрезом
    double t, x, y;

    Point p1(cutter[0].x, cutter[0].y), p2(cutter[1].x, cutter[1].y);
    Point p3(shell[iCut-1].x, shell[iCut-1].y), p4(shell[iCut].x, shell[iCut].y);

    t = ((p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x)) / ((p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y));
    x = p1.x + t * (p2.x - p1.x);
    y = p1.y + t * (p2.y - p1.y);

    Point firstInter(x, y);

    p3.x = shell[jCut-1].x;
    p3.y = shell[jCut-1].y;
    p4.x = shell[jCut].x;
    p4.y = shell[jCut].y;

    t = ((p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x)) / ((p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y));
    x = p1.x + t * (p2.x - p1.x);
    y = p1.y + t * (p2.y - p1.y);

    Point secondInter(x, y);

    //удаление точек, которые лежат в той полуплоскости, где нет ядра
    while (true)
    {
        if (shell[iCut].x == jCutPoint.x && shell[iCut].y == jCutPoint.y)
            break;

        if (iCut == (shell.size() - 1))
        {
            shell.pop_back();
            iCut = 0;
        }

        shell.erase(shell.begin() + iCut);
    }

    //добавление к точкам оболочки точек пересечения
    if (iCut == 0)
    {
        shell.push_back(firstInter);
        shell.push_back(secondInter);
        shell.push_back(shell[0]);
    }
    else
    {
        shell.insert(shell.begin() + iCut, secondInter);
        shell.insert(shell.begin() + iCut, firstInter);
    }
}

void mouse(int button, int state, int x, int y)
{
    //инициировано начало игры
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP && !isStart)
    {
        makeMeteor();

        cutter.clear();
        cutter.push_back(Point(1, -8.7));
        cutter.push_back(Point(1, 8.7));

        isStart = true;
        isFirstCutterFixed = false;
        isSecondCutterFixed = false;
        blueScore = 0;
        redScore = 0;
        playerColor = 0;
    }

    //фиксируем точку резчика на нижнем слайдере
    if (isStart
        && !isFirstCutterFixed
        && ((button == GLUT_LEFT_BUTTON && playerColor % 2 == 0) || (button == GLUT_RIGHT_BUTTON && playerColor % 2 == 1))
        && state == GLUT_DOWN)
    {
        isFirstCutterFixed = true;
    }

    //фиксируем точку резчика на верхнем слайдере
    if (isStart
        && isFirstCutterFixed
        && !isSecondCutterFixed
        && ((button == GLUT_LEFT_BUTTON && playerColor % 2 == 0) || (button == GLUT_RIGHT_BUTTON && playerColor % 2 == 1))
        && state == GLUT_UP)
    {
        isShellCut = isCut(shell);
        isCoreCut = isCut(core);

        isSecondCutterFixed = true;
    }
}

void timerFunc(int value)
{
    //анимация движения точки резчика по нижнему слайдеру
    if (isStart && !isFirstCutterFixed && !isSecondCutterFixed)
    {
        if (sliderStep >= 9)
            sliderCycleSign = -1;

        if (sliderStep <= -9)
            sliderCycleSign = 1;

        sliderStep += 0.05 * sliderCycleSign;
        cutter[0].x = sliderStep;

        glutPostRedisplay();
    }

    //анимация движения точки резчика по верхнему слайдеру
    if (isStart && isFirstCutterFixed && !isSecondCutterFixed)
    {
        if (sliderStep >= 9)
            sliderCycleSign = -1;

        if (sliderStep <= -9)
            sliderCycleSign = 1;

        sliderStep += 0.05 * sliderCycleSign;
        cutter[1].x = sliderStep;

        glutPostRedisplay();
    }

    //анимация разреза и логический блок с определением победителя и продолжением игры
    if (isStart && isFirstCutterFixed && isSecondCutterFixed)
    {
        if (1 - cycleVariable <= EPS)
        {
            isFirstCutterFixed = false;
            isSecondCutterFixed = false;

            playerColor ++;
            cycleVariable = 0;

            if (!isShellCut || isCoreCut)
            {
                (playerColor % 2 != 0) ? blueScore ++ : redScore ++;

                if (redScore == MAX_SCORE || blueScore == MAX_SCORE)
                {
                    isStart = false;
                    glutPostRedisplay();
                }
                else
                {
                    makeMeteor();
                }
            }
            else
            {
                cutMeteor();
            }
        }

        //анимация разреза ядра
        if (isCoreCut && (int) (cycleVariable * 1000) % 29 == 0)
        {
            glColor3f(0.9, 1 - cycleVariable, 1 - cycleVariable);

            glBegin(GL_POLYGON);
            for (double i = 0; i < core.size() - 1; i++)
            {
                glVertex2f(core[i].x + (double) (rand() % 10 - 5) / 30,
                           core[i].y + (double) (rand() % 10 - 5) / 30);
            }
            glEnd();
        }

        //анимация промоха
        if (!isShellCut)
        {
            glColor3f(1 - cycleVariable, 1 - cycleVariable, 1 - cycleVariable);

            glBegin(GL_POLYGON);
            for (double i = 0; i < shell.size() - 1; i++)
            {
                glVertex2f(shell[i].x, shell[i].y);
            }
            glEnd();
        }

        cycleVariable += 0.004;

        //переливание линии разреза
        drawCut(cutter[0], cutter[1], cycleVariable, 7);
        drawCut(cutter[0], cutter[1], (1 - cycleVariable) / 2, 4);
        drawCut(cutter[0], cutter[1], 1 - cycleVariable, 2);

        glFlush();
    }

    glutTimerFunc(timerFrequency, timerFunc, 0);
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(500, 50);
    glutCreateWindow("ME7E0R CU7TER");
    glShadeModel(GL_FLAT);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutTimerFunc(0, timerFunc, 0);

    glutMainLoop();
    return 0;
}
