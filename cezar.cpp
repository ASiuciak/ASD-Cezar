#include <stdio.h>
#include <string>
#include <vector>

#define BIG 1000000007 // 10^9 + 7

using namespace std;

struct SplayNode
{
    SplayNode * up, * left, * right;
    int key; //klucz używany tylko przy budowie

    // Liczniki węzłów w poddrzewach, wykorzystywane do wyszukiwania
    // k-tego w aktualnym inorder węzła
    int count_left;
    int count_right;
    char c; // R lub G
    bool rev; // flaga informująca, czy poddrzewo jest odwrócone

    // Wartości zliczające prawidłowe patrole dla fragmentu reprezentowanego przez węzeł i jego poddrzewo.
    // RR - liczba prawidłowych patroli zaczynających i kończących się Rzymianinem, RG, GR, GG - analogicznie
    long long RR;
    long long RG;
    long long GR;
    long long GG;
};

// Wstawia węzły do drzewa przy jego budowie (preprocessing only).
// Kolejność ustalona za pomocą kluczy używanych tylko przy budowie drzewa.
void insert(SplayNode * & root, int k, char c)
{
    SplayNode * w, * p;
    w = new SplayNode;
    w->left = w->right = nullptr;
    w->c = c;
    p = root;
    if(!p) {root = w;}
    else {
        while (true)
            if (k < p->key) {
                if (!p->left) {
                    p->left = w;
                    break;
                } else p = p->left;
            } else {
                if (!p->right) {
                    p->right = w;
                    break;
                } else p = p->right;
            }
    }
    w->up  = p;
    w->key = k;
}

// Ustawia RR, RG, GR i GG na odpowiadające połączonemu przedziałowi left + right.
void update(SplayNode * & left, SplayNode * & right, long long *RR, long long *RG, long long *GR, long long *GG) {
    *RR = (left->RR + right->RR + left->RR * right->RR + left->RR * right->GR + left->RG * right->RR) % BIG;
    *RG = (left->RG + right->RG + left->RR * right->RG + left->RR * right->GG + left->RG * right->RG) % BIG;
    *GR = (left->GR + right->GR + left->GR * right->RR + left->GR * right->GR + left->GG * right->RR) % BIG;
    *GG = (left->GG + right->GG + left->GR * right->RG + left->GR * right->GG + left->GG * right->RG) % BIG;
}

// Aktualizuje wartości RR, RG, GR i GG dla węzła
void updateNode(SplayNode * & x) {
    SplayNode * left = x->left;
    SplayNode * right = x->right;
    long long RR, RG, GR, GG;
    RR = 0;
    RG = 0;
    GR = 0;
    GG = 0;
    if (x->c == 'R') {
        x->RR = 1;
        x->RG = 0;
        x->GR = 0;
        x->GG = 0;
    }
    else {
        x->RR = 0;
        x->RG = 0;
        x->GR = 0;
        x->GG = 1;
    }
    if (left != nullptr) {
        update(left, x, &RR, &RG, &GR, &GG);
        x->RR = RR;
        x->RG = RG;
        x->GR = GR;
        x->GG = GG;
    }
    if (right != nullptr) {
        update(x, right, &RR, &RG, &GR, &GG);
        x->RR = RR;
        x->RG = RG;
        x->GR = GR;
        x->GG = GG;
    }
}

// Aktualizuje liczniki węzłów w poddrzewach.
void updateCounters(SplayNode * & x){
    if(x->left != nullptr) {
        x->count_left = x->left->count_right + x->left->count_left + 1;
    }
    else {
        x->count_left = 0;
    }

    if(x->right != nullptr) {
        x->count_right = x->right->count_right + x->right->count_left + 1;
    }
    else {
        x->count_right = 0;
    }
}

// "Spycha" flagę reverse do synów i zamienia ich miejscami.
void reverse(SplayNode* & x) {
    if (x != nullptr && x->rev) {
        x->rev = false;
        SplayNode* temp = x->left;
        x->left = x->right;
        x->right = temp;
        size_t t2 = x->count_left;
        x->count_left = x->count_right;
        x->count_right = t2;
        if (x->left != nullptr) {
            x->left->rev = !x->left->rev;
            long long t1 = x->left->RG;
            x->left->RG = x->left->GR;
            x->left->GR = t1;
        }
        if (x->right != nullptr) {
            x->right->rev = !x->right->rev;
            long long t1 = x->right->RG;
            x->right->RG = x->right->GR;
            x->right->GR = t1;
        }
        updateNode(x);
    }
}

// Rotacja w lewo
void rotateLeft(SplayNode * & root, SplayNode * A)
{
    SplayNode * B = A->right, * p = A->up;
    reverse(B);
    if(B)
    {
        A->right = B->left;
        if(A->right) { A->right->up = A;}

        B->left = A;
        B->up = p;
        A->up = B;

        if(p)
        {
            if(p->left == A) p->left = B; else p->right = B;
        }
        else { root = B;}
        updateCounters(A);
        updateNode(A);
        updateCounters(B);
        updateNode(B);
    }
}

// Rotacja w prawo.
void rotateRight(SplayNode * & root, SplayNode * A)
{
    SplayNode * B = A->left, * p = A->up;
    reverse(B);
    if(B)
    {
        A->left = B->right;
        if(A->left) { A->left->up = A;}

        B->right = A;
        B->up = p;
        A->up = B;

        if(p)
        {
            if(p->left == A) p->left = B; else p->right = B;
        }
        else { root = B;}
        updateCounters(A);
        updateNode(A);
        updateCounters(B);
        updateNode(B);
    }
}

// Procedura splay, ustawia wskazany węzeł x jako korzeń drzewa o korzeniu root,
// zachowuje przy tym inorder.
// Dzięki funkcjom updateCounters i updateNode umieszczonych w rotacjach,
// węzły zachowują również prawidłowe informacje o liczbie odpowiednich patroli
// i liczebności poddrzew.
void splay(SplayNode * & root, SplayNode * & x)
{
    if (root == nullptr || x == nullptr) { return;}
    while(true)
    {
        reverse(x);
        if(x == root) break;
        reverse(x->up->left);
        reverse(x->up->right);
        if(x->up == root)
        {
            if(x->up->left == x) { rotateRight(root, x->up);}
            else { rotateLeft(root, x->up);}
            break;
        }
        reverse(x->up->up->left);
        reverse(x->up->up->right);
        if((x->up->up->left == x->up) && (x->up->left == x))
        {
            rotateRight(root, x->up->up);
            rotateRight(root, x->up);
            continue;
        }
        if((x->up->up->right == x->up) && (x->up->right == x))
        {
            rotateLeft(root, x->up->up);
            rotateLeft(root, x->up);
            continue;
        }
        if(x->up->right == x)
        {
            rotateLeft(root, x->up);
            rotateRight(root, x->up);
        }
        else {
            rotateRight(root, x->up);
            rotateLeft(root, x->up);
        }
    }
}

// Odcina lewe poddrzewo, zwraca jego korzeń
SplayNode* splitLeft(SplayNode * & root){
    SplayNode * res = root->left;
    if (res != nullptr) {res->up = nullptr;}
    root->left = nullptr;
    updateCounters(root);
    updateNode(root);
    return res;
}

// Odcina prawe poddrzewo, zwraca jego korzeń
SplayNode* splitRight(SplayNode * & root){
    SplayNode * res = root->right;
    if (res != nullptr) {res->up = nullptr;}
    root->right = nullptr;
    updateCounters(root);
    updateNode(root);
    return res;
}

// Tworzy zrównawożone drzewo splay o danej liczbie węzłów i ich wartościach (R / G).
void even (SplayNode* & root, const string & army, int value, int up_limit, int down_limit) {
    insert(root, value, army[value - 1]);
    int left = (value + down_limit) / 2;
    if (left > down_limit && left < value) {
        even(root, army, left, value, down_limit);
    }
    int right = (value + up_limit) / 2;
    if (right < up_limit && right > value) {
        even(root, army, right, up_limit, value);
    }
}

// Oblicza początkowe wartości RR, RG, GR, GG w węzłach oraz
// ustawia początkowe liczebności lewych i prawych poddrzew.
void preprocessing(SplayNode* & root){
    if (root != nullptr){
        preprocessing(root->left);
        preprocessing(root->right);
        updateNode(root);
        updateCounters(root);
        root->rev = false;
    }
}

// Tworzy nowe drzewo splay z wpisanymi poprawnie parametrami.
SplayNode* newBalancedTree(const int & n, const string & army) {
    SplayNode * root = nullptr;
    even(root, army, (n + 1) / 2, n + 1, 0);
    preprocessing(root);
    return root;
}

// Szuka k-tego węzła w aktualnym porządku inorder:
SplayNode* find(SplayNode* & root, const int & k){
    if (root == nullptr) { return nullptr;}
    if (k < 0 || k > root->count_left + root->count_right + 1) { return nullptr;}
    SplayNode* x = root;
    reverse(x);
    if (x->count_left < k - 1) {
        return find(x->right, (k - x->count_left - 1));
    }
    else if (x->count_left == k - 1) {
        return x;
    }
    return find(x->left, k);
}

// Funkcja zamieniająca kolejność żołnierzy na danym odcinku [i,j].
void Otask (SplayNode* & root, const int & i, const int & j) {
    if (i >= j) { return;}
    // Splayujemy i-ty węzeł i odcinamy lewe poddrzewo (lower trzyma korzeń lewego poddrzewa)
    SplayNode* first = find(root, i);
    splay(root, first);
    SplayNode* lower = splitLeft(root);
    reverse(lower);

    // Splayujemy j-ty węzeł (po odcięciu lewego jest on na pozycji j - i + 1) i odcinamy prawe poddrzewo
    SplayNode* last = find(root, j - i + 1);
    splay(root, last);
    SplayNode* greater = splitRight(root);
    reverse(greater);

    root->rev = true;

    // Splayujemy największy węzeł z lewego drzewa (o ile nie jest ono NULLem)
    // Prawym synem korzenia lewego drzewa staje się wtedy korzeń drzewa środkowego
    SplayNode* maxLow = find(lower, i - 1);
    root->up = maxLow;
    if (maxLow != nullptr) {
        splay(lower, maxLow);
        maxLow->right = root;
        root = maxLow;
    }
    updateCounters(root);
    updateNode(root);

    // Splayujemy połączone (lewe + środkowe) drzewo - ostatni element idzie na górę.
    last = find(root, j);
    splay(root, last);

    // Prawym synem połączonego drzewa staje się korzeń prawego drzewa.
    root->right = greater;
    if (greater != nullptr) {greater->up = root;}
    updateCounters(root);
    updateNode(root);
}

// Funkcja obsługująca zapytanie o ilość godnych zaufania patroli na przedziale [i,j].
void question(SplayNode* & root, const int & i, const int & j) {
    if (i >= j) {
        printf("%d\n", 1);
        return;
    }
    // Splayujemy i-ty węzeł i odcinamy lewe poddrzewo (lower trzyma korzeń lewego poddrzewa)
    SplayNode* first = find(root, i);
    splay(root, first);
    SplayNode* lower = splitLeft(root);
    reverse(lower);

    // Splayujemy j-ty węzeł (po odcięciu lewego jest on na pozycji j - i + 1) i odcinamy prawe poddrzewo
    SplayNode* last = find(root, j - i + 1);
    splay(root, last);
    SplayNode* greater = splitRight(root);
    reverse(greater);

    long long patrols = (root->RR + root->RG + root->GR + root->GG) % BIG;

    // Splayujemy największy węzeł z lewego drzewa (o ile nie jest ono NULLem)
    // Prawym synem korzenia lewego drzewa staje się wtedy korzeń drzewa środkowego
    SplayNode* maxLow = find(lower, i - 1);
    root->up = maxLow;
    if (maxLow != nullptr) {
        splay(lower, maxLow);
        maxLow->right = root;
        root = maxLow;
    }

    updateCounters(root);
    updateNode(root);

    // Splayujemy połączone (lewe + środkowe) drzewo - ostatni element idzie na górę.
    last = find(root, j);
    splay(root, last);

    // Prawym synem połączonego drzewa staje się korzeń prawego drzewa.
    root->right = greater;
    if (greater != nullptr) {greater->up = root;}
    updateCounters(root);
    updateNode(root);
    // Wypisujemy wynik.
    printf("%d\n", patrols);
}

// Zwalnia pamięć zaalokowaną na drzewo.
void freeTree(SplayNode * & root)
{
    if(root != nullptr)
    {
        freeTree(root->left);
        freeTree(root->right);
        delete root;
    }
}


int main()
{
    int n, m;
    scanf("%d", &n);
    scanf("%d", &m);
    string army;
    cin >> army;
    SplayNode* root = newBalancedTree(n, army);
    for (int k = 0; k < m; k ++) {
        char task;
        int i, j;
        cin >> task;
        scanf("%d", &i);
        scanf("%d", &j);
        if (task == '?') {
            question(root, i, j);
        }
        if (task == 'O') {
            Otask(root, i, j);
        }
    }
    return 0;
}
