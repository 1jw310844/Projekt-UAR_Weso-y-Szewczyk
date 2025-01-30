#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "oknogenerator.h"
#include "oknoregulator.h"
#include "oknoobiektarx.h"
#include <QDebug>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>

std::random_device srng;
std::mt19937 rng;


void MainWindow::UstawienieLayout(){
    qDebug() << "Ustawianie layoutu";
    for(int i=0;i<4;i++){
        layout[i] = new QVBoxLayout();
        layout[i]->setContentsMargins(0, 0, 0, 0);
    }
    ui->wykresWarZad->setLayout(layout[0]);
    ui->WykUchyb->setLayout(layout[1]);
    ui->WykPID->setLayout(layout[2]);
    ui->WykSter->setLayout(layout[3]);
    qDebug() << "Layout ustawiony";
}

MainWindow::MainWindow(QWidget *parent, WarstwaUslug *prog)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , simulationTimer(new QTimer(this))
    ,usluga(prog)
    ,czas(0.0)
{
    qDebug() << "MainWindow constructor";
    usluga->setGUI(this);
    ui->setupUi(this);
    wykres=new  Wykresy();
    okno_gen = new OknoGenerator(this);
    okno_gen->setWarstwaUslug(usluga);
    okno_reg = new OknoRegulator(this);
    okno_reg->setWarstwaUslug(usluga);
    okno_obiekt = new OknoObiektARX(this);
    okno_obiekt->setWarstwaUslug(usluga);

    this->showMaximized();
    simulationTimer = new QTimer(this);

    UstawienieGUI();
   // fileSelected("");


    //symulator* s=new symulator;

    UstawienieLayout();
    wykres->InicjalizujWykresy(layout);
    connect(usluga, &WarstwaUslug::PoprawneDane, this, &MainWindow::PokazWykres);
    connect(usluga, &WarstwaUslug::BledneDane, this, &MainWindow::Blad);
    connect(usluga, &WarstwaUslug::sygnalZapisano, this, &MainWindow::obslugaZapisu);
    qDebug() << "Sygnały połączone";
    Wczytaj = new QPushButton("Wczytaj konfigurację", this);
    connect(Wczytaj, &QPushButton::clicked, this, &MainWindow::on_Wczytaj_clicked);
    //ui->Layout->addWidget(Wczytaj);  // Dodanie do układu przycisków
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_Start_clicked()
{
    qDebug() << "Przycisk Start kliknięty";
    usluga->SprawdzenieWszystkichDanych(interwalCzasowy);
    qDebug() << "Sprawdzenie wszystkich danych zakończone";
}

void MainWindow::on_Stop_clicked()
{
    qDebug() << "Przycisk Stop kliknięty";
    simulationTimer->stop();
    qDebug() << "Timer zatrzymany";
}

void MainWindow::on_Interwal_textChanged(const QString &arg1)
{
    qDebug() << "Zmiana interwału czasowego";

    interwalCzasowy=arg1.toInt();
    qDebug() << "Nowy interwał czasowy: " << interwalCzasowy;

}

void MainWindow::PokazWykres(symulator* s) {
    if (!s) {
        qDebug() << "symulator* s jest nullptr!";
        return;
    }

    qDebug() << "Inicjalizacja wykresów";
     // Ustawienie symulatora
    wykres->setSymulator(s); // Ustawienie symulatora dla obiektu wykresów

    // Inicjalizowanie wykresów dla każdego layoutu


    // Połączenie timera z funkcją aktualizującą wykresy

    connect(simulationTimer, &QTimer::timeout, this, [=]() {
        wykres->WykresWartosciZadanej();
    });
    connect(simulationTimer, &QTimer::timeout, this, [=]() {
        wykres->WykresUchybu();
    });
    connect(simulationTimer, &QTimer::timeout, this, [=]() {
        wykres->WykresPID();
    });
    connect(simulationTimer, &QTimer::timeout, this, [=]() {
        wykres->WykresWartosciSterowania();
    });

    // Uruchomienie timera, jeśli interwał czasowy jest większy niż 0
    if (interwalCzasowy > 0) {
        simulationTimer->start(interwalCzasowy);  // Rozpoczęcie timera
        qDebug() << "Timer uruchomiony z interwałem " << interwalCzasowy;
    } else {
        qDebug() << "Błędny interwał czasowy";
    }
}



void MainWindow::Blad(){
    qDebug() << "Wywołano funkcję Blad()";
    QMessageBox::warning(this, "Ostrzeżenie", "Nie uzupełniłeś Wszystkich Danych");
}

void MainWindow::on_UstawieniaGeneratora_clicked()
{
    qDebug() << "Ustawienia Generatora kliknięte";
    okno_gen->exec();
}

void MainWindow::on_UstawieniaObiektuARX_clicked()
{
    qDebug() << "Ustawienia Obiektu ARX kliknięte";
    okno_obiekt->exec();
}

void MainWindow::on_UstawieniaRegulatora_clicked()
{
    qDebug() << "Ustawienia Regulatora kliknięte";
    okno_reg->exec();
}

void MainWindow::on_Reset_clicked()
{
    // Zatrzymanie timera i odłączenie sygnałów
    disconnect(simulationTimer, nullptr, nullptr, nullptr);
    simulationTimer->stop();

    // Resetowanie wykresów i czasu
    wykres->ResetujWykresy();
    wykres->ResetCzas();

    // Resetowanie zmiennych symulatora
    symulator* sym = usluga->getSymulator();
    if (sym) {
        sym->getRegulator().ZerowanieNastawaP();
        sym->getRegulator().ZerowanieNastawaI();
        sym->getRegulator().ZerowanieNastawaD();
        sym->setWyjscieObiektu(0);
        sym->setLastRegulatorValue(0);
        sym->setLastObjectOutput(0);

        // Reset generatora sygnału
        Generator gen = sym->getGenerator();
        gen.setAmplituda(0);
        gen.setOkres(1);
        gen.setWypelnienie(0.5);
        sym->setGenerator(gen);

        // Reset regulatora
        Regulator reg;
        sym->setRegulator(reg);

        // Reset obiektu ARX
        ObiektARX obiekt;
        sym->setObiektARX(obiekt);
    }
    // Resetowanie GUI
    czas = 0;
    interwalCzasowy = 0;
    isSimulationRunning = false;

    // Ponowna inicjalizacja wykresów
    wykres->InicjalizujWykresy(layout);

    qDebug() << "Symulacja i wykresy zostały zresetowane.";
}

void MainWindow::UstawienieGUI(){
    QString buttonStyleStart =
        "QPushButton {"
        "    background-color: white;"
        "    color: black;"
        "    border-radius: 10px;"
        "    border: 2px solid black;"
        "    padding: 5px;"
        "    transition: 0.5s ease-in-out;"

        "}"
        "QPushButton:hover {"
        "    background-color: green;"
        "    color: white;"
        "    padding: 8px;"
        "    border: 3px solid #fff;"
        "}";
    QString buttonStyleStop =
        "QPushButton {"
        "    background-color: white;"
        "    color: black;"
        "    border-radius: 10px;"
        "    border: 2px solid black;"
        "    padding: 5px;"
        "    transition: 0.5s ease-in-out;"
        "}"
        "QPushButton:hover {"
        "    background-color: red;"
        "    color: white;"
        "    padding: 8px;"
        "    border: 3px solid #fff;"
        "}";
    QString buttonStyleReset =
        "QPushButton {"
        "    background-color: white;"
        "    color: black;"
        "    border-radius: 10px;"
        "    border: 2px solid black;"
        "    padding: 5px;"
        "    transition: 0.5s ease-in-out;"
        "}"
        "QPushButton:hover {"
        "    background-color: orange;"
        "    color: white;"
        "    padding: 8px;"
        "    border: 3px solid #fff;"
        "}";
    QString buttonStyleReszta =
        "QPushButton {"
        "    background-color: white;"
        "    color: black;"
        "    border-radius: 10px;"
        "    border: 2px solid black;"
        "    padding: 5px;"
        "    transition: 0.5s ease-in-out;"
        "}"
        "QPushButton:hover {"
        "    background-color: grey;"
        "    color: white;"
        "    padding: 8px;"
        "    border: 3px solid #fff;"
        "}";

    this->setStyleSheet(
        "QWidget { color: white; }"
        "QMainWindow { background-color: rgb(90,90,90); }"
        );
    this->setStyleSheet(
        "QMessageBox { color: black; "
        "background-color: grey;"
        "}"
        "QMainWindow { background-color: rgb(90,90,90); }"
        );

    ui->Interwal->setStyleSheet(
        "QLineEdit {"
        "    background-color: white;"
        "    color: black;"
        "}"
        );
    ui->Start->setStyleSheet(buttonStyleStart);
    ui->Stop->setStyleSheet(buttonStyleStop);
    ui->Reset->setStyleSheet(buttonStyleReset);
    ui->Wczytaj->setStyleSheet(buttonStyleReszta);
    ui->UstawieniaGeneratora->setStyleSheet(buttonStyleReszta);
    ui->UstawieniaRegulatora->setStyleSheet(buttonStyleReszta);
    ui->UstawieniaObiektuARX->setStyleSheet(buttonStyleReszta);
    ui->Zapisz->setStyleSheet(buttonStyleReszta);

    ui->TytulWykres1->setStyleSheet(
        "QLabel {"
        "    background-color: rgb(130, 130, 130);"
        "    color: white;"
        "    border: 1px solid black;"
        "    padding: 5px;"
        "}"
        );

    ui->TytulWykres2->setStyleSheet(
        "QLabel {"
        "    background-color: rgb(130, 130, 130);"
        "    color: white;"
        "    border: 1px solid black;"
        "    padding: 5px;"
        "}"
        );

    ui->TytulWykres3->setStyleSheet(
        "QLabel {"
        "    background-color: rgb(130, 130, 130);"
        "    color: white;"
        "    border: 1px solid black;"
        "    padding: 5px;"
        "}"
        );

    ui->TytulWykres4->setStyleSheet(
        "QLabel {"
        "    background-color: rgb(130, 130, 130);"
        "    color: white;"
        "    border: 1px solid black;"
        "    padding: 5px;"
        "}"
        );

    ui->TytulGlowny->setStyleSheet(
        "QLabel {"
        "    background-color: rgb(130, 130, 130);"
        "    color: white;"
        "    border: 1px solid black;"
        "    padding: 5px;"
        "}"
        );
    ui->InterwalCzasowyLabel->setStyleSheet(
        "QLabel {"
        "    background-color: rgb(130, 130, 130);"
        "    color: white;"
        "    border: 1px solid black;"
        "    padding: 5px;"
        "}"
        );

    QGraphicsDropShadowEffect *effect[8]; //

    for (int i = 0; i < 8; ++i) {
        effect[i] = new QGraphicsDropShadowEffect();
        effect[i]->setOffset(0, 0);           // Ustawienie przesunięcia cienia (0, 0 dla cienia wokół)
        effect[i]->setBlurRadius(10);         // Rozmycie cienia
        effect[i]->setColor(Qt::black);       // Kolor cienia
    }
    ui->Start->setGraphicsEffect(effect[0]);
    ui->Stop->setGraphicsEffect(effect[1]);
    ui->Reset->setGraphicsEffect(effect[2]);
    ui->UstawieniaGeneratora->setGraphicsEffect(effect[3]);
    ui->UstawieniaObiektuARX->setGraphicsEffect(effect[4]);
    ui->UstawieniaRegulatora->setGraphicsEffect(effect[5]);
    ui->Zapisz->setGraphicsEffect(effect[6]);
    ui->Wczytaj->setGraphicsEffect(effect[7]);
}

void MainWindow::obslugaZapisu()
{
    QMessageBox::information(this, "Zapis konfiguracji", "Konfiguracja została zapisana.");
    qDebug() << "Odebrano sygnał zapisu.";
}

void MainWindow::on_Zapisz_clicked()
{
    usluga->zapiszKonfiguracje();
    QMessageBox::information(this, "Sukces", "Konfiguracja została zapisana.");
    /*
    Generator gen = usluga->getSymulator()->getGenerator();
    Regulator reg = usluga->getSymulator()->getRegulator();
    ObiektARX obiekt = usluga->getSymulator()->getObiektARX();
    QFile file("konfiguracja.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Generator:\n";
        out << "Amplituda: " << gen.getAmplituda() << "\n";
        out << "Okres: " << gen.getOkres() << "\n";
        out << "Wypelnienie: " << gen.getWypelnienie() << "\n";
        out << "Rodzaj Sygnalu: " << static_cast<int>(gen.getRodzaj()) << "\n";
        out << "\nRegulator:\n";
        out << "Wzmocnienie: " << reg.getWzmocnienie() << "\n";
        out << "Stala I: " << reg.getStalaI() << "\n";
        out << "Stala D: " << reg.getStalaD() << "\n";
        out << "\nObiekt ARX:\n";
        out << "Opoznienie: " << obiekt.getOpoznienie() << "\n";
        out << "Współczynniki A: ";
        for (auto a : obiekt.getWielomianA()) {
            out << a << " ";
        }
        out << "\nWspółczynniki B: ";
        for (auto b : obiekt.getWielomianB()) {
            out << b << " ";
        }
        out << "\n";
        file.close();
        QMessageBox::information(this, "Sukces", "Konfiguracja została zapisana do pliku.");
    } else {
        QMessageBox::warning(this, "Błąd", "Nie udało się otworzyć pliku do zapisu.");
    }
*/
}

void MainWindow::on_Wczytaj_clicked()
{
    usluga->wczytajKonfiguracje();
    QMessageBox::information(this, "Sukces", "Konfiguracja została wczytana.");
    /*
    QFile file("konfiguracja.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Błąd", "Nie udało się otworzyć pliku.");
        return;
    }

    QTextStream in(&file);
    Generator gen;
    Regulator reg;
    ObiektARX obiekt;
    std::vector<double> A_coeffs, B_coeffs;
    double amp, okres, wyp, wz, stI, stD, opoznienie;

    QString line;
    while (!in.atEnd()) {
        line = in.readLine();
        if (line.startsWith("Amplituda:")) amp = line.split(": ")[1].toDouble();
        if (line.startsWith("Okres:")) okres = line.split(": ")[1].toDouble();
        if (line.startsWith("Wypelnienie:")) wyp = line.split(": ")[1].toDouble();
        if (line.startsWith("Rodzaj Sygnalu:")) gen.setRodzaj(static_cast<RodzajSygnalu>(line.split(": ")[1].toInt()));

        if (line.startsWith("Wzmocnienie:")) wz = line.split(": ")[1].toDouble();
        if (line.startsWith("Stala I:")) stI = line.split(": ")[1].toDouble();
        if (line.startsWith("Stala D:")) stD = line.split(": ")[1].toDouble();

        if (line.startsWith("Opoznienie:")) opoznienie = line.split(": ")[1].toDouble();

        if (line.startsWith("Współczynniki A:")) {
            QStringList values = line.split(": ")[1].split(" ");
            for (QString val : values) A_coeffs.push_back(val.toDouble());
        }
        if (line.startsWith("Współczynniki B:")) {
            QStringList values = line.split(": ")[1].split(" ");
            for (QString val : values) B_coeffs.push_back(val.toDouble());
        }
    }
    file.close();

    // Ustawienie wartości w obiektach
    gen.setAmplituda(amp);
    gen.setOkres(okres);
    gen.setWypelnienie(wyp);

    reg.setWzmocnienie(wz);
    reg.setStalaI(stI);
    reg.setStalaD(stD);

    obiekt.setOpoznienie(opoznienie);
    obiekt.setWielomianA(A_coeffs);
    obiekt.setWielomianB(B_coeffs);

    // Ustawienie w symulatorze
    symulator* sym = usluga->getSymulator();
    if (sym) {
        sym->setGenerator(gen);
        sym->setRegulator(reg);
        sym->setObiektARX(obiekt);
    }
    QMessageBox::information(this, "Sukces", "Konfiguracja została wczytana.");
*/
}
