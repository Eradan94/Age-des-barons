#include <stdio.h>
#include <stdlib.h>
#include <MLV/MLV_all.h>
#include <time.h>

// dimension du monde en nombre de cases 
#define LONG 18
#define HAUT 12
// l’origine est en haut à gauche 

// dimension en pixels de la fenetre 
#define TAILLE_C 50
#define TAILLE_X (LONG + 5) * TAILLE_C
#define TAILLE_Y (HAUT + 1) * TAILLE_C

// les deux clans 
#define ROUGE ’R’
#define BLEU  ’B’

// les genres d’agents 
#define MANANT ’m’
#define BARON  ’b’
#define GUERRIER  ’g’
#define CHATEAU ’c’

// les temps de production 
#define TMANANT 2
#define TGUERRIER 4
#define TBARON 6

// les couts 
#define CMANANT 1
#define CGUERRIER 5
#define CBARON 10
#define CCHATEAU 30

typedef struct agent{
	char couleur; // ROUGE ou BLEU 
	char genre; // BARON, MANANT, GUERRIER, CHATEAU 
	int posx, posy; // position actuelle 
	int destx, desty; // destination (negatif si immobile) 
	char produit; // production actuelle d’un chateau 
	int temps; // tours restant pour cette production 
	struct agent *asuiv, *aprec; // liste des agents liees a un chateau 
	struct agent *vsuiv, *vprec; // liste des voisins 
}Agent, *AListe;


typedef struct{
	Agent *chateau; // s’il y a un chateau sur la case 
	AListe habitant; // les autres occupants 
}Case;


typedef struct{
	Case plateau[LONG][HAUT];
	AListe rouge, bleu;
	int tour; // Numero du tour 
	int tresorRouge, tresorBleu;
}Monde;

void affiche_grille_jeu(){
	//Cette fonction affiche une grille de 12 cases de largeur et 18 de longueur
	
	int x;
	int y;
	for(x = 0; x < 19; x++){
		//On affiche les lignes
		MLV_draw_line((x * TAILLE_C), 0, (x * TAILLE_C), (HAUT * TAILLE_C), MLV_COLOR_WHITE);
	}
	for(y = 0; y < 13; y++){
		//On affiche les colonnes
		MLV_draw_line(0, (y * TAILLE_C), (LONG * TAILLE_C), (y * TAILLE_C), MLV_COLOR_WHITE);
	}
	//On met à jour la fenêtre pour afficher la grille
	MLV_actualise_window();
}

AListe alloue_agent(char couleur, char genre) {
	//Cette fonction alloue l'espace mémoire pour un agent et l'initialise, elle prend en argument la couleur de l'agent et son genre. Elle renvoie l'adresse de l'agent
	
	//On initialise un agent à NULL
	AListe tmp = NULL;

	//On alloue l'espace mémoire pour l'agent
	tmp = (AListe)malloc(sizeof(Agent));
	//Si on a assez de memoire on peut initialiser l'agent
	if (tmp != NULL) {
		//On initialise la couleur, le genre de l'agent, temps = 0 car cet agentvient d'être créé
		tmp -> couleur = couleur;
		tmp -> genre = genre;
		tmp -> temps = 0;
		//Si l'agent est un chateau, les coorodnées de sa destination sont négatives car le chateau est immobile
		if (genre == 'c') {
			tmp -> destx = -1;
			tmp -> desty = -1;
		}
		//On initialise les asuiv, aprec, vsuiv, et vprec à NULL pour eviter des erreurs de segmentations par la suite.
		tmp -> asuiv = NULL;
		tmp -> vsuiv = NULL;
		tmp -> aprec = NULL;
		tmp-> vprec = NULL;
	}
	//On renvoie l'aresse de l'agent initialisé
	return tmp;
}

AListe insertion_triee(Monde * partie, char genre, AListe chateau){
	//Cette fonction alloue et insère un agent dans une liste, elle prend en argument un Monde, le genre de l'agent et un chateau et renvoie l'adresse de l'agent
	
	//On initialise deux listes : n_ag qui est l'agent créé et tmp, une temporaire
	AListe tmp;
	AListe n_ag;

	//On alloue l'agent selon sa couleur
	if(chateau -> couleur == 'R'){
		n_ag = alloue_agent('R', genre);
	}
	else{
		n_ag = alloue_agent('B', genre);
	}
	
	//On place la temporaire sur le chateau, on pourra docn parcourir la liste des agents liés à ce chateau
	tmp = chateau;
	//Si l'agent est un manant alors on parcourt la liste jusqu'à ce qu'elle soit nulle ou que le genre de la temporaire soit différent de manant
	if (genre == 'm') {
		while(tmp->asuiv != NULL && tmp->genre != genre){
			tmp =  tmp->asuiv;
		}
	}
	//Si l'agent est un guerrier alors on parcourt la liste jusqu'à ce qu'elle soit nulle ou que le genre de tmp soit différent de guerrier, ou jusqu'à ce que l'on arrive sur un manant 
	//car le gurrier est placé avant le manant
	else if (genre == 'g') {
		while(tmp->asuiv != NULL && tmp->genre != genre && tmp->asuiv->genre != 'm'){
			tmp =  tmp->asuiv;
		}
	}
	//Si l'agent est un baron on parcourt la liste jusqu'à ce qu'elle soit nulle ou jusqu'à ce que l'on arrive sur un manant ou un guerrier car les barons sont placés avant les guerriers et les manants
	else if (genre == 'b') {
		while(tmp->asuiv != NULL && tmp->genre != genre && tmp->asuiv->genre != 'g' && tmp->asuiv->genre != 'm'){
			tmp =  tmp->asuiv;
		}
	}
	if(tmp->asuiv != NULL){
		// Si le voisin n'est pas NULL alors on insère l'agent entre deux autres agents
		n_ag->asuiv = tmp->asuiv;
		tmp->asuiv = n_ag;
		n_ag->asuiv->aprec = n_ag;
	}
	else{
		//Sinon l'agent est placé en dernière position
		tmp->asuiv = n_ag;
	}
	//Le précédant de l'agent est la temporaire placée sur l'agent le précédant
	n_ag->aprec = tmp;
	//On renvoie l'adresse de l'agent créé
	return n_ag;
}

void ajoute_voisin(Monde *partie, Agent *ag, int x, int y){
	//Cette fonction est similaire à la focntion d'insertion, mais prend en argument des coordonnées au lieu d'un chateau, elle insère un agent déja existant dans le voisinage d'une case
	
	//On initialise une temporaire
	AListe tmp;
	//Cette temporaire pointe sur l'habitant de la case voulue, cette agent est le premier agent qui participera aux combats en cas d'attaque
	tmp = partie->plateau[x][y].habitant;
	
	//On place d'abord tous les agents d'une couleur puis tous les agents de l'autre equipe
	//On vérifie donc si la couleur l'agent que l'on veut insérer est bien la même que celle du premier voisin de la case
	if(tmp->couleur == ag->couleur){
		//Si c'est le cas, on place l'agent avant ceux de l'autre équipe
		
		//Si le premier voisin est un manant et que l'agent que l'on veut inserer n'en est pas un, ou que le premier voisin est un baron et que l'agent que l'on insere est un guerrier alors on le place avant
		if((tmp->genre == 'm' && ag->genre != 'm')||(tmp->genre == 'b' && ag->genre =='g')){
				//Dans ce cas là on insère en tête
				ag->vsuiv = tmp;
				//l'agent pris en argument devient alors le premier voisin de cette case, c'est lui qui interviendra en premier en cas de combat
				partie->plateau[x][y].habitant = ag;
				tmp->vprec = ag;
				//On renvoie rien pour ne pas continuer la fonction, l'agent est déja inséré
				return;
		}
		//Sinon on fait comme dans la fonction d'insertion, mais les barons sont placés après les guerriers
		if (ag->genre == 'm') {
			while(tmp->vsuiv != NULL && tmp->genre != ag->genre && tmp->vsuiv->couleur == ag->couleur){
				tmp =  tmp->vsuiv;
			}
		}
		else if (ag->genre == 'b') {
			while(tmp->vsuiv != NULL && tmp->genre != ag->genre && tmp->vsuiv->genre != 'm'){
				tmp =  tmp->vsuiv;
			}
		}
		else if (ag->genre == 'g') {
			while(tmp->vsuiv != NULL && tmp->genre != ag->genre && tmp->vsuiv->genre != 'g' && tmp->vsuiv->genre != 'm'){
				tmp =  tmp->vsuiv;
			}
		}
		if(tmp->vsuiv != NULL){
			// Si le voisin n'est pas NULL alors on insère l'agent entre deux autres agents
			ag->vsuiv = tmp->vsuiv;
			tmp->vsuiv = ag;
			ag->vsuiv->aprec = ag;
		}
		else{
			//Sinon on l'insère en fin de liste
			tmp->vsuiv = ag;
		}
		//Le précédant de l'agent est la temporaire placée sur l'agent le précédant
		ag->vprec = tmp;
	}
	//Dans le cas ou l'agent pris en argument n'est pas de la même couleur que le premier voisin
	else{
		//Alors on se place en fin de liste
		while(tmp->couleur != ag->couleur && tmp->vsuiv != NULL){
			tmp = tmp-> vsuiv;
		}
		//Si le dernier agent de la liste n'est toujours pas de la même équipe alors on l'inèsere en fin de liste
		if(tmp->couleur != ag->couleur){
			tmp->vsuiv = ag;
			ag->vprec = tmp;
			//On renvoie rien, pour ne pas faire le reste de la fonction
			return;
		}
		//Si le premier voisin est un manant et que l'agent que l'on veut inserer n'en est pas un, ou que le premier voisin est un baron et que l'agent que l'on inserer est un gurrier alors on le place avant
		if((tmp->genre == 'm' && ag->genre != 'm')||(tmp->genre == 'b' && ag->genre =='g')){
			// Dans ce cas là l'agent est placé entre le dernier agent de l'équipe adverse et le premier agent de son équipe
			ag->vsuiv = tmp;
			ag->vprec = tmp->vprec;
			tmp->vprec = ag;
			ag->vprec->vsuiv = ag; 
			//On renvoie rien pour ne pas continuer la fonction
			return;
		}
		//Sinon on fait les mêmes tests que la première partie de la fonction
		if (ag->genre == 'm') {
			while(tmp->vsuiv != NULL && tmp->genre != ag->genre){
				tmp =  tmp->vsuiv;
			}
		}
		else if (ag->genre == 'b') {
			while(tmp->vsuiv != NULL && tmp->genre != ag->genre && tmp->vsuiv->genre != 'm'){
				tmp =  tmp->vsuiv;
			}
		}
		else if (ag->genre == 'g') {
			while(tmp->vsuiv != NULL && tmp->genre != ag->genre && tmp->vsuiv->genre != 'g' && tmp->vsuiv->genre != 'm'){
				tmp =  tmp->vsuiv;
			}
		}
		if(tmp->vsuiv != NULL){
			ag->vsuiv = tmp->vsuiv;
			tmp->vsuiv = ag;
			ag->vsuiv->aprec = ag;
		}
		else{
			tmp->vsuiv = ag;
		}
		ag->vprec = tmp;
	}
}

void place_agent_cree(Monde *partie,  char genre, AListe chateau){
	//Cette fonction alloue, insère, et place l'agent créé sur la case du chateau qui l'a créé, elle prend en argument un Monde, le genre de l'agent et un chateau
	
	//On alloue et insère l'agent dans la liste des agents du chateau
	AListe ag = insertion_triee(partie, genre, chateau);
	//Si il n'y a pas d'habitant sur la case du chateau alors on place l'agent dessus, il devient le premier voisin de cette case
	if(partie->plateau[chateau->posx][chateau->posy].habitant == NULL){
		partie->plateau[chateau->posx][chateau->posy].habitant = ag;
	}
	//Sinon, il y a déja des habitants sur cette case, on l'insère dans ce voisinage
	else{
		ajoute_voisin(partie, ag, chateau->posx, chateau->posy);
	}
	//On donne comme position et comme destination à l'agent , les coordonnées du chateau qui l'a créé
	ag -> posx = chateau->posx;
	ag -> posy = chateau->posy;
	ag -> destx = chateau->posx;
	ag -> desty = chateau->posy;
}

Case alloue_case(Agent * chateau, AListe habitant) {
	//Cette fonction alloue l'espace mémoire pour une case, elle prend en argument un chateau et une liste d'agent et renvoie l'adresse de la case
	
	//On définit une variable de type Case
	Case* cel;
	//On alloue de l'espace mémoire pour cette case
	cel = (Case *)malloc(sizeof(Case));
	if (cel != NULL) {
		//Si on peut allouer de la mémoire pour cette case alors on initialise son habitant et son chateau (à NULL si les deux arguments sont NULL)
		cel -> chateau = chateau;
		cel -> habitant = habitant;
	}
	//On renvoie l'adresse de la cellule
	return *cel;
}

void init_plateau(Monde * partie) {
	//Cette fonction alloue chaque case du plateau
	int i;
	int j;

	for(i = 0; i < LONG; i++) {
		for(j = 0; j < HAUT; j++) {
			//Pour chaque case du plateau on lui alloue de l'espace mémoire et on initialise son habitant et son chateau à NULL
			partie -> plateau[i][j] = alloue_case(NULL, NULL);
		}
	}
}

void init_monde(Monde * partie){
	//Cette fonction initialise la partie, elle prend donc en argument un Monde
	
	//On initialise tout d'abord le plateau
	init_plateau(partie);
	
	//On alloue ensuite un chateau pour l'equipe rouge
	partie->rouge = alloue_agent('R', 'c');
	//On place ce chateau sur la case de coordonnées [0,0]
	partie -> plateau[0][0].chateau = partie -> rouge;
	//On met à jour ses coorodnnées
	partie -> rouge -> posx = 0;
	partie -> rouge -> posy = 0;
	//partie -> rouge -> destx = 0;
	//partie -> rouge -> desty = 0;
	
	//On créé deux agents, un baron et un manant qui seront placés sur la case du chateau
	place_agent_cree(partie, 'b', partie->rouge);
	place_agent_cree(partie, 'm', partie->rouge);
	
	//Même chose pour l'équipe bleue
	partie->bleu = alloue_agent('B', 'c');	
	partie -> plateau[LONG - 1][HAUT - 1].chateau = partie -> bleu;
	partie -> bleu -> posx = LONG -1;
	partie -> bleu -> posy = HAUT - 1;
	//partie -> bleu -> destx = LONG -1;
	//partie -> bleu -> desty = HAUT - 1;
	
	place_agent_cree(partie, 'b', partie->bleu);
	place_agent_cree(partie, 'm', partie->bleu);
	
	//Le trésor de chaque équipe est initialisé à 50 et le tour à 0
	partie -> tour = 0;
	partie -> tresorRouge = 50;
	partie -> tresorBleu = 50;
}

void affiche_bouton(char genre) {
	//Cette fonction affiche des boutons pour donner des ordres
	MLV_draw_filled_rectangle((LONG * TAILLE_C) + 1, 2 * TAILLE_C + 1, 5 * TAILLE_C, HAUT * TAILLE_C, MLV_COLOR_BLACK);
	if (genre == 'c') {
		//Si l'agent est un chateau on affiche créé et affiche des boxs qui serviront à creer des manants, des barons ou des guerriers
		MLV_draw_text_box(
			(LONG + 1) * TAILLE_C,
			3 * TAILLE_C,
			3 * TAILLE_C,
			TAILLE_C,
			"Manant\n%dT %dPO",
			TAILLE_C / 6,
			MLV_COLOR_WHITE,
			MLV_COLOR_WHITE,
			MLV_COLOR_BLACK,
			MLV_TEXT_CENTER,
			MLV_HORIZONTAL_CENTER,
			MLV_VERTICAL_CENTER,
			TMANANT, CMANANT
		);
		MLV_draw_text_box(
			(LONG + 1) * TAILLE_C,
			5 * TAILLE_C,
			3 * TAILLE_C,
			TAILLE_C,
			"Guerrier\n%dT %dPO",
			TAILLE_C / 6,
			MLV_COLOR_WHITE,
			MLV_COLOR_WHITE,
			MLV_COLOR_BLACK,
			MLV_TEXT_CENTER,
			MLV_HORIZONTAL_CENTER,
			MLV_VERTICAL_CENTER,
			TGUERRIER, CGUERRIER
		);
		MLV_draw_text_box(
			(LONG + 1) * TAILLE_C,
			7 * TAILLE_C,
			3 * TAILLE_C,
			TAILLE_C,
			"Baron\n%dT %dPO",
			TAILLE_C / 6,
			MLV_COLOR_WHITE,
			MLV_COLOR_WHITE,
			MLV_COLOR_BLACK,
			MLV_TEXT_CENTER,
			MLV_HORIZONTAL_CENTER,
			MLV_VERTICAL_CENTER,
			TBARON, CBARON
		);
		MLV_draw_text_box(
			(LONG + 1) * TAILLE_C,
			9 * TAILLE_C,
			3 * TAILLE_C,
			TAILLE_C,
			"Ne rien faire",
			TAILLE_C / 6,
			MLV_COLOR_WHITE,
			MLV_COLOR_WHITE,
			MLV_COLOR_BLACK,
			MLV_TEXT_CENTER,
			MLV_HORIZONTAL_CENTER,
			MLV_VERTICAL_CENTER
		);
	}
	else {
		if(genre == 'b'){
			//Si l'agent est un baron on affiche un bouton pour créer un chateau
			MLV_draw_text_box(
				(LONG + 1) * TAILLE_C,
				5 * TAILLE_C,
				3 * TAILLE_C,
				TAILLE_C,
				"Créer un chateau",
				TAILLE_C / 6,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_BLACK,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
		}
		MLV_draw_text_box(
			//On ajoute aussi un bouton pour disperser l'agent ainsi qu'un bouton pour passer son tour
			(LONG + 1) * TAILLE_C,
			3 * TAILLE_C,
			3 * TAILLE_C,
			TAILLE_C,
			"Dispertion",
			TAILLE_C / 6,
			MLV_COLOR_WHITE,
			MLV_COLOR_WHITE,
			MLV_COLOR_BLACK,
			MLV_TEXT_CENTER,
			MLV_HORIZONTAL_CENTER,
			MLV_VERTICAL_CENTER
		);
		MLV_draw_text_box(
			(LONG + 1) * TAILLE_C,
			9 * TAILLE_C,
			3 * TAILLE_C,
			TAILLE_C,
			"Ne rien faire",
			TAILLE_C / 6,
			MLV_COLOR_WHITE,
			MLV_COLOR_WHITE,
			MLV_COLOR_BLACK,
			MLV_TEXT_CENTER,
			MLV_HORIZONTAL_CENTER,
			MLV_VERTICAL_CENTER
		);
	}
	MLV_draw_text_box(
		(LONG + 1) * TAILLE_C,
		11 * TAILLE_C,
		3 * TAILLE_C,
		TAILLE_C,
		"Fin du tour",
		TAILLE_C / 6,
		MLV_COLOR_WHITE,
		MLV_COLOR_WHITE,
		MLV_COLOR_BLACK,
		MLV_TEXT_CENTER,
		MLV_HORIZONTAL_CENTER,
		MLV_VERTICAL_CENTER
	);
	//On affiche ces boutons
	MLV_actualise_window();
}

void affiche_contenu(Monde partie){
	//Cette fonction affiche les agents présent dans la partie
	
	//On initialise une temporaire pour parcourir le plateau
	AListe tmp;
	int i;
	int j;
	
	//On définit les images utilisées pour afficher les agents
	MLV_Image *image_chateau_bleu;
	MLV_Image *image_manant_bleu;
	MLV_Image *image_baron_bleu;
	MLV_Image *image_guerrier_bleu;

	MLV_Image *image_chateau_rouge;
	MLV_Image *image_manant_rouge;
	MLV_Image *image_baron_rouge;
	MLV_Image *image_guerrier_rouge;
	
	image_chateau_bleu = MLV_load_image("images/chateau_bleu.png");
	image_baron_bleu = MLV_load_image("images/baron_bleu.png");
	image_guerrier_bleu = MLV_load_image("images/guerrier_bleu.png");
	image_manant_bleu = MLV_load_image("images/manant_bleu.png");
	
	image_chateau_rouge = MLV_load_image("images/chateau_rouge.png");
	image_baron_rouge = MLV_load_image("images/baron_rouge.png");
	image_guerrier_rouge = MLV_load_image("images/guerrier_rouge.png");
	image_manant_rouge = MLV_load_image("images/manant_rouge.png");

	MLV_resize_image(image_chateau_rouge, TAILLE_C / 2 - 1, TAILLE_C / 2 - 1);
	MLV_resize_image(image_manant_rouge, TAILLE_C / 2 - 1, TAILLE_C / 2 - 1);
	MLV_resize_image(image_baron_rouge, TAILLE_C / 2 - 1, TAILLE_C / 2 - 1);
	MLV_resize_image(image_guerrier_rouge, TAILLE_C / 2 - 1, TAILLE_C / 2 - 1);
	
	MLV_resize_image(image_chateau_bleu, TAILLE_C / 2 - 1, TAILLE_C / 2 - 1);
	MLV_resize_image(image_manant_bleu, TAILLE_C / 2 - 1, TAILLE_C / 2 - 1);
	MLV_resize_image(image_baron_bleu, TAILLE_C / 2 - 1, TAILLE_C / 2 - 1);
	MLV_resize_image(image_guerrier_bleu, TAILLE_C / 2 - 1, TAILLE_C / 2 - 1);

	//On efface le précédant contenu de la fenêtre, pour que les agens supprimés lors du dernier tour ne soient plus affichés
	MLV_clear_window(MLV_COLOR_BLACK);
	//On affiche d'abord la grille
	affiche_grille_jeu();

	for(i = 0; i < LONG; i++) {
		for(j = 0; j < HAUT; j++) {
			//On traite chaque case du plateau, on vérifie d'abord que l'habitant ne soit pas NULL
			if(partie.plateau[i][j].habitant != NULL){
				//S'il n'est pas NULL, on place la temporaire sur l'habitant de cette case
				tmp = partie.plateau[i][j].habitant;
				//Ensuite on parcourt la liste de ses voisins tant que la temporaire n'est pas NULL
				while(tmp != NULL){
					//Pour chaque type d'agent de telle couleur on affiche l'image correspondante
					if(tmp->genre == 'b'){
						if(tmp-> couleur == 'B'){
							MLV_draw_image(image_baron_bleu, (i*TAILLE_C)+((TAILLE_C/2)+1), (j*TAILLE_C)+1);
						}
	
						else{
							MLV_draw_image(image_baron_rouge, (i*TAILLE_C)+((TAILLE_C/2)+1), (j*TAILLE_C)+1);
						}					
					}				
					if(tmp->genre == 'm'){
						if(tmp-> couleur == 'B'){
							MLV_draw_image(image_manant_bleu, (i*TAILLE_C)+((TAILLE_C/2)+1), (j*TAILLE_C)+((TAILLE_C/2)+1));
						}
						else{
							MLV_draw_image(image_manant_rouge, (i*TAILLE_C)+((TAILLE_C/2)+1), (j*TAILLE_C)+((TAILLE_C/2)+1));						
						}
					}

					if(tmp->genre == 'g'){
						if(tmp-> couleur == 'B'){
							MLV_draw_image(image_guerrier_bleu, (i*TAILLE_C)+1, (j*TAILLE_C)+((TAILLE_C/2)+1));
						}
						else{
							MLV_draw_image(image_guerrier_rouge, (i*TAILLE_C)+1, (j*TAILLE_C)+((TAILLE_C/2)+1));
						
						}					
					}
					//On passe au voisin suivant
					tmp = tmp->vsuiv;
				}
			}
			//On vérifie aussi que le chateau ne soit pas NULL
			if(partie.plateau[i][j].chateau != NULL){
				if(partie.plateau[i][j].chateau-> couleur == 'B'){
					MLV_draw_image(image_chateau_bleu, (i*TAILLE_C)+1, (j*TAILLE_C)+1);
				}
				else{
					MLV_draw_image(image_chateau_rouge, (i*TAILLE_C)+1, (j*TAILLE_C)+1);
				}					
			}
			
		}
	}
	//On met à jour la fenêtre
	MLV_actualise_window();

	// Puis on libère l'espace utilisé pas les images
	MLV_free_image(image_chateau_rouge);
	MLV_free_image(image_manant_rouge);
	MLV_free_image(image_baron_rouge);
	MLV_free_image(image_guerrier_rouge);
	
	MLV_free_image(image_chateau_bleu);
	MLV_free_image(image_manant_bleu);
	MLV_free_image(image_baron_bleu);
	MLV_free_image(image_guerrier_bleu);
}

void affiche_agent(Agent ag) {
	//Cette fonction affiche le genre, la position, la destination d'un agent
	printf("genre : %c\n", ag.genre);
	if (ag.genre == 'm') {
		printf("Manant : \n  Position : (%d, %d)\n  Destination : (%d, %d)\n", ag.posx, ag.posy, ag.destx, ag.desty);
	}
	else if (ag.genre == 'g') {
		printf("Guerrier : \n  Position : (%d, %d)\n  Destination : (%d, %d)\n", ag.posx, ag.posy, ag.destx, ag.desty);
	}
	else if (ag.genre == 'b') {
		printf("Baron : \n  Position : (%d, %d)\n  Destination : (%d, %d)\n", ag.posx, ag.posy, ag.destx, ag.desty);
	}
	else if (ag.genre == 'c') {
		printf("Château : \n  Production : %c\n  Temps restant : %d\n", ag.produit, ag.temps);
	}
}

void supprimer_agent(AListe ag, Monde * partie){
	//Cette fonction supprime un agent de la partie, elle prend en argument un Monde et un agent
	
	//On définit ptmp qui est l'agent précédant
	AListe ptmp;
	ptmp = ag->aprec;
	//Le suivant de cet agent précédant a pour suivant, le suivant de l'agent à supprimer
	ptmp->asuiv = ptmp->asuiv->asuiv;
	//Si le suivant de l'agent à supprimer n'est pas NULL alors le précédant de son suivant est son précédant
	if(ag->asuiv != NULL) ag->asuiv->aprec = ptmp;
	//On met son suivant à NULL
	ag->asuiv = NULL;	
	//On libère l'espace mémoire
	free(ag);
}

void supprimer_voisin(AListe ag, Monde *partie){
	//Cette fonction supprime un agent d'un voisinage, elle prend en argument un Monde et un agent
	
	if(ag->vprec == NULL){
		//Si cet agent n'a pas de vprec alors il est le premier voisin
		//Dans ce cas, le case pointe sur son voisin suivant
		partie -> plateau[ag->posx][ag->posy].habitant = ag->vsuiv;
		//On me le voisin précédant de son voisin suivant à NULL, car il n'a plus de voisin précédant : il est le premier voisin
		ag->vsuiv->vprec = NULL;
		//On met son voisin suivant à NULL
		ag->vsuiv = NULL;
	}
	else{
		//Sinon, cet agent n'est pas le premier voisin
		//Dans ce cas, si il n'est pas le dernier agent de la liste, le vprec de son vsuiv est son vprec
		if(ag->vsuiv != NULL) ag->vsuiv->vprec = ag->vprec;
		//Le vsuiv de son vprec devient son vsuiv
		ag->vprec->vsuiv = ag->vsuiv;
		ag->vprec = NULL;
		ag->vsuiv = NULL;
	}
	//A la fin de cette fonction, l'agent n'a plus de vsuiv ni de vprec, il ne fait plus parti d'un voisinage
}

AListe supprime_chateau(Monde *partie, AListe chateau){
	//Cette fonction supprime un chateau de la partie, elle prend en argument un Monde et un chateau et renvoie une liste contenant ses agents
	
	//On définit deux temporaires
	AListe ptmp;
	AListe tmp;
	if(chateau -> couleur == 'B'){
		//Si le chateau pris en argument n'est pas le même que le premier alors ptmp pointe sur son vprec
		if(chateau != partie -> bleu){
			ptmp = chateau -> vprec;
		}
		//Sinon, le chateau à supprimer est le premierde l'équipe, il n'a pas de vprec
		else{
			//Si ce chateau n'a pas de vsuiv, alors son équipe n'a plus qu'un seul chateau
			if(chateau -> vsuiv == NULL){
				//Dans ce cas, l'équipe a perdu, la liste de l'équipe pointe vers NULL
				partie->bleu = NULL;
				//On libère l'espace mémoire
				free(chateau);
				//On renvoie NULL, comme l'équipe a perdu, il est inutile de rattacher ses manants à l'autre équipe ou de supprimer ses agents
				return NULL;
			}
			//Sinon le chateau est le premier chateau de son équipe, mais il n'est pas le seul
			//Dans ce cas la liste de l'équipe pointe sur le chateau suivant	
			partie->bleu = partie->bleu->vsuiv;
			//Le chateau suivant n'a plus de précédant
			partie->bleu->vprec = NULL;
			//On initialise tmp sur le premier agent de la liste des agents liés au chateau
			tmp = chateau->asuiv;
			//Le chateau n'a plus de suivant ni de d'agent
			chateau->vsuiv = NULL;
			chateau->asuiv = NULL;
			//On libère l'espace mémoire
			free(chateau);
			//On renvoie l'adresse du premier agent de la liste des agents du chateau
			return tmp;
		}
	}
	//Même chose pour l'autre équipe
	else{
		if(chateau != partie -> rouge){
			ptmp = chateau -> vprec;
		}
		else{
			if(chateau -> vsuiv == NULL){
				partie->rouge = NULL;
				free(chateau);
				return NULL;
			}
			partie->rouge = partie->rouge->vsuiv;
			partie->rouge->vprec = NULL;
			tmp = chateau;
			chateau->vsuiv = NULL;
			tmp = chateau->asuiv;
			chateau->asuiv = NULL;
			free(chateau);
			return tmp;
		}
	}
	//Si le chateau n'est pas le premier de son équipe on vérifie si il est le dernier ou non
	if(chateau -> asuiv == NULL){
		//Si le chateau n'a pas d'agents on extrait le chateau de la liste et on renvoie NULL
		ptmp->vsuiv = ptmp-> vsuiv -> vsuiv;
		free(chateau);
		return NULL;
	}
	//On place tmp sur le premier agent du chateau
	tmp = chateau->asuiv;
	//On extrait le chateau de la liste
	ptmp -> vsuiv = ptmp -> vsuiv -> vsuiv;
	//Le chateau n'a plus d'agents
	chateau -> asuiv = NULL;
	//On libère l'espace mémoire
	free(chateau);
	// On renvoie l'adresse du premier agent de liste des agents du chateau
	return tmp;
}

void rattache_agent(Monde *partie, AListe lst_agent, Agent *ag){
	//Cette fonction supprime et rattache les agents d'un chateau détruit, elle prend en argument un Monde, une liste d'agent, et un agent
	//Les manants de la lise d'agents seront ajoutés à celle de l'agent qui a detruit le chateau
	
	//On initialise une temporaire
	AListe tmp;
	//On crée une boucle qui parcourt la liste jusqu'à ce que tmp soit NULL ou jusqu'à ce que tmp pointe sur un manant
	while(lst_agent != NULL && lst_agent->genre != 'm'){
		//On initialise tmp au début de la liste
		tmp = lst_agent;
		//Si l'agent a un voisin on le supprime d'abord de son voisinage
		if(tmp->vprec != NULL || tmp-> vsuiv != NULL){
			supprimer_voisin(tmp, partie);
		}
		//Sinon il est seul sur la case, on remet l'habitant de la case à NULL
		else{
			partie->plateau[tmp->posx][tmp->posy].habitant = NULL;
		}
		//On passe à l'agent suivant et on libère l'espace mémoire de l'agent précédant
		lst_agent = lst_agent->asuiv;
		free(tmp);
	}
	//Si la liste est NULL alors il est inutile de continuer
	if(lst_agent == NULL){
		return;
	}
	else{
		//Sinon il faut rattacher les manants à la liste de l'agent qui a detruit le chateau
		tmp = lst_agent;
		while(tmp != NULL){
			//On change la couleur de chaque manant et leur destination, pour pouvoir leur donner de nouveaux ordres
			tmp->couleur = ag->couleur;
			tmp->destx = tmp->posx;
			tmp->desty = tmp->posy;
			tmp = tmp->asuiv;
		}
		//On va à la fin de la liste de l'agent qui a detruit le chateau et on y colle le début de la liste des manants à rattacher
		while(ag->asuiv != NULL) ag = ag->asuiv;
		ag->asuiv = lst_agent;
	}
}

void affiche_liste_equipe(AListe equipe){
	//Cette fonction affiche les agents d'une équipe
	
	//On place deux temporaires au début de l'equipe
	AListe tmp;
	AListe tmpc;
	tmp = equipe;
	tmpc = equipe;
	printf("Liste des agents de l'équipe :\n");
	
	//On parcourt chaque chateau de l'équipe, et pour chaque chateau, on affiche ses agents
	while(tmpc != NULL){
		while(tmp != NULL){
			affiche_agent(*tmp);
			//On passe à l'agent suivant
			tmp = tmp -> asuiv;
		}
		//On passe au chateau suivant et tmp pointe sur ce chateau
		tmpc = tmpc -> vsuiv;
		tmp = tmpc;
	}
	printf("\n");
}

int test_ordre(Agent agent) {
	if (agent.genre == 'c') {
		if (agent.temps == 0) return 1;
		else return 0;
	}

	if (agent.genre == 'm') {
		if (agent.destx < 0 && agent.desty < 0) return 0;
		else {
			if ((agent.posx == agent.destx) && (agent.posy == agent.desty)) return 1;
			else return 0;
		}
	}

	if ((agent.posx == agent.destx) && (agent.posy == agent.desty)) return 1;
	else return 0;
}

int tirage_aleatoire(int debut, int fin){
	//Cette fonction renvoie un entier aléatoire entre debut et fin exclu
	
	return rand()%(fin - debut) + debut;
}

int affrontement(Agent *ag1, Agent *ag2){
	//Cette fonction permet de résoudre l'issue d'un affrontement entre deux agents, elle renvoie -1 si ag1 perd, 1 si ag1 gagne et 0 en cas d'égalité
	
	//On définit deux scores, un pour chaque agent
	int score_ag1; 
	int score_ag2;
	//Si le genre de l'agent est baron alors son score est un nombre aléatoire entre 0 et 100 inclus multiplié par 6
	//Si le genre de l'agent est guerrier alors son score est un nombre aléatoire entre 0 et 100 inclus multiplié par 4
	//Si le genre de l'agent est manant son score est de 0
	if(ag1->genre == 'b'){
		score_ag1 = tirage_aleatoire(0, 101)*6;
	}
	else if (ag1->genre == 'g') score_ag1 = tirage_aleatoire(0, 101)*4;
	else score_ag1 = 0;
	
	
	if(ag2->genre == 'b'){
		score_ag2 = tirage_aleatoire(0, 101)*6;
	}
	else if(ag2->genre == 'g') score_ag2 = tirage_aleatoire(0, 101)*4;
	else score_ag2 = 0;
	
	//Si les deux agents sont des manants, un combat se déroule quand même, leur score est compris entre 0 et 100
	if(ag1->genre == 'm' && ag2->genre == 'm'){
		score_ag1 = tirage_aleatoire(0, 101);
		score_ag2 = tirage_aleatoire(0, 101);
	}
	//On renvoie 0, 1, -1 selon l'issue du combat
	if(score_ag1 > score_ag2) return 1;
	if(score_ag1 == score_ag2) return 0;
	return -1;
}

//Il s'agit d'une fonction non utilisée, servant à afficher la liste de voisinage d'une case
void affiche_liste_case(Monde *partie, int x, int y){
	AListe tmp;
	tmp = partie->plateau[x][y].habitant;
	while(tmp != NULL){
		printf("agent : %c, %c : ", tmp->genre, tmp->couleur);
		if(tmp->vsuiv != NULL){
			printf("Suivant : %c %c ", tmp->vsuiv->genre, tmp->vsuiv->couleur);
		}
		if(tmp->vprec != NULL){
			printf("Precedent : %c %c", tmp->vprec->genre, tmp->vprec->couleur);
		}
		printf("\n");
		tmp = tmp->vsuiv;
	}
	printf("\n");
}

void combat(Monde *partie, int x, int y){
	//Cette fonction sert à résoudre les combats entre deux équipes, elle prend en argument un Monde, et les coordonnées du combat
	
	//On initialise deux temporaires, une qui sert à parcourir la liste de voisins sur la case, et l'autre qui sert à pointer sur d'éventuelles listes d'agents au cas où un chateau est détruit
	AListe tmp;
	AListe atmp;
	AListe lst_agent = NULL;
	//On initialise une variable pour le resultat de l'affrontement entre deux agents
	int res;
	//On initialise une autre variable qui contient la couleur du premier agent du voisinage
	char couleur;
	//On place tmp sur le premier agent du voisinage
	tmp = partie->plateau[x][y].habitant;
	//La variable couleur représente la couleur de l'équipe en défense
	couleur = partie->plateau[x][y].habitant->couleur;
	//La boucle continue jsuqu'à ce que l'on utilise un return;
	//On parcourt la liste jusqu'au premier agent de l'équipe adverse
	while(tmp->vsuiv != NULL && tmp->couleur == partie->plateau[x][y].habitant->couleur ){		
		tmp = tmp->vsuiv;
	}
	while(1){
		//Si tmp est NULL alors l'équipe adverse est vaincue, on casse la boucle
		if(tmp == NULL) break;
		if(partie->plateau[x][y].habitant->couleur == couleur){
			//Sinon il y a toujours des affrontements, le premier de l'equipe bleue et le premier de l'equipe rouge se battent
			res = affrontement(partie->plateau[x][y].habitant, tmp);
			//Selon l'issue du combat on supprime tel agent, sans oublier de le supprimer du voisinage
			if(res == -1){
			//Si le premier agent de la liste est supprimé, la liste pointe sur son suivant
				atmp = partie->plateau[x][y].habitant;
				supprimer_voisin(partie->plateau[x][y].habitant, partie);
				supprimer_agent(atmp, partie); 
			}	
			if(res == 1){
				atmp = tmp;
				tmp = tmp->vsuiv;
				supprimer_voisin(atmp, partie);
				supprimer_agent(atmp, partie);
			}	
		}
		//Si l'agent sur lequel la case pointe n'est pas de la même couleur que couleur alors l'équipe en défense a perdu.
		else break;
	}
	//On vérifie si il y a un chateau et si il n'est pas de la même couleur que les defenseurs, si non, alors il n'y a rien à faire
	if(partie->plateau[x][y].chateau != NULL && partie->plateau[x][y].habitant-> couleur != couleur){
		//On supprime donc le chateau et rattache les agents qui etaient liés à ce chateau
		lst_agent = supprime_chateau(partie, partie->plateau[x][y].chateau);
		//On met le chateau de la case à NULL
		partie->plateau[tmp->posx][tmp->posy].chateau = NULL;
		if(lst_agent != NULL){
			rattache_agent(partie, lst_agent, partie->plateau[x][y].habitant);
		}
		//Le combat est fini
	}
}

int detecte_ennemi(Monde partie, Agent ag, int x, int y){
	//Cette fonction prend en argument un Monde, un agent et une direction, elle teste le contenu de la case en question et renvoie telle valeur pour tel contenu
	// 0 si la case ne contient rien
	// 1 si la case contient un agent adverse
	// -1 si la case contient un agent allié
	// 2 si la case contient un chateau ennemi
	// -2 si la case contient un chateau allié
	
	if(partie.plateau[(ag.posx)+x][(ag.posy)+y].habitant != NULL){
	    if(partie.plateau[(ag.posx)+x][(ag.posy)+y].habitant->couleur != ag.couleur) return 1;
	    else return -1;
	}
	if(partie.plateau[(ag.posx)+x][(ag.posy)+y].chateau != NULL){
		 if(partie.plateau[(ag.posx)+x][(ag.posy)+y].chateau->couleur != ag.couleur) return 2;
		 else return -2;
	}
	return 0;
}

void direction(Monde *partie, Agent *ag, int x, int y){
	//Cette fonction effectue le mouvement d'un agent sur une case à proximité, elle prend en argument un Monde, un agent et une direction
	
	//Si l'agent n'a pas de voisin alors l'habitant de sa case devient NULL
	if(ag->vsuiv == NULL && ag->vprec == NULL) partie->plateau[ag->posx][ag->posy].habitant = NULL;
	//Sinon, il a un voisin, on supprime l'agent de ce voisinage
	else supprimer_voisin(ag, partie);
	//On met à jour ses coordonnées et la l'habitant de la case où il va pointe sur lui
	partie->plateau[(ag->posx)+x][(ag->posy)+y].habitant = ag;	
	ag->posx = (ag->posx) +x;
	ag->posy = (ag->posy) +y;
	ag->vsuiv = NULL;
	ag->vprec = NULL;
}

int teste_manant(Monde partie, Agent manant){
	// Cette fonction teste s'il y a un manant qui recolte sur la case du manant donné en argument.
	AListe tmp = partie.plateau[manant.posx][manant.posy].habitant;

	// On parcourt la liste des agents de la case
	while (tmp != NULL) {
		if (tmp -> genre == 'm') {
			if (tmp -> destx == -1 && tmp -> desty == -1) {
				// Si l'agent courant est un manant et que sa destination est (-1, -1) alors il recolte
				// donc le manant passé en argument ne peux pas recolter lui aussi.
				return 1;
			}
		}
		tmp = tmp -> vsuiv;
	}
	return 0;
}

void deplacement_agent(Monde *partie, Agent *ag){
	//Cette fonction effectue les deplacements des agents, elle prend en argument un Monde et l'agent à deplacer
	//On teste d'abord chaque direction en comparant destx avec posx et desty avec posy
	if(ag->posx != ag->destx && ag->posy != ag->desty && ag->posx > ag->destx && ag->posy > ag->desty){
		//Si l'habitant de la case est NULL alors l'agent peut se deplacer dessus
		if(detecte_ennemi(*partie, *ag, -1, -1) == 0  || ((detecte_ennemi(*partie, *ag, -1, -1) == -2 || detecte_ennemi(*partie, *ag, -1, -1) == 2) && partie->plateau[ag->posx -1][ag->posy -1].habitant == NULL)){
			//On appelle alors direction pour déplacer l'agent sur cette case
			direction(partie, ag, -1, -1);
		}
		else{
			//Si l'agent n'est pas seul sur sa case alors on le supprime de son voisinage
			if(partie->plateau[ag->posx][ag->posy].habitant->vsuiv != NULL){
				supprimer_voisin(ag, partie);
			}
			else{
				//Sinon il est seul sur sa case, on met cette case à NULL
				partie->plateau[ag->posx][ag->posy].habitant = NULL;
			}
			//L'agent peut se déplacer mais comme il y a déja un agent sur cette case on doit d'abord l'ajouter dans le voisinage de la case
			//On met à jour les coordonnées de l'agent
			ajoute_voisin(partie, ag, ag->posx -1, ag->posy -1);
			ag->posx = (ag->posx) -1;
			ag->posy = (ag->posy) -1;
		}
	}
	else if(ag->posx != ag->destx && ag->posy != ag->desty && ag->posx < ag->destx && ag->posy < ag->desty){
		if(detecte_ennemi(*partie, *ag, 1, 1) == 0  || ((detecte_ennemi(*partie, *ag, 1, 1) == -2 || detecte_ennemi(*partie, *ag, 1, 1) == 2) && partie->plateau[ag->posx +1][ag->posy +1].habitant == NULL)){
			direction(partie, ag, 1, 1);
		}
		else{
			if(partie->plateau[ag->posx][ag->posy].habitant->vsuiv != NULL){
				supprimer_voisin(ag, partie);
			}
			else{
				partie->plateau[ag->posx][ag->posy].habitant = NULL;
			}
			ajoute_voisin(partie, ag, ag->posx +1, ag->posy +1);
			ag->posx = (ag->posx) +1;
			ag->posy = (ag->posy) +1;
		}
	}
	else if(ag->posx != ag->destx && ag->posy != ag->desty && ag->posx < ag->destx && ag->posy > ag->desty){
		if(detecte_ennemi(*partie, *ag, 1, -1) == 0  || ((detecte_ennemi(*partie, *ag, 1, -1) == -2 || detecte_ennemi(*partie, *ag, 1, -1) == 2)&& partie->plateau[ag->posx +1][ag->posy -1].habitant == NULL)){
			direction(partie, ag, 1, -1);
		}
		else{
			if(partie->plateau[ag->posx][ag->posy].habitant->vsuiv != NULL){
				supprimer_voisin(ag, partie);
			}
			else{
				partie->plateau[ag->posx][ag->posy].habitant = NULL;
			}
			ajoute_voisin(partie, ag, ag->posx +1, ag->posy -1);
			ag->posx = (ag->posx) +1;
			ag->posy = (ag->posy) -1;
		}
	}
	else if(ag->posx != ag->destx && ag->posy != ag->desty && ag->posx > ag->destx && ag->posy < ag->desty){
		if(detecte_ennemi(*partie, *ag, -1, 1) == 0  || ((detecte_ennemi(*partie, *ag, -1, 1) == -2 || detecte_ennemi(*partie, *ag, -1, 1) == 2) && partie->plateau[ag->posx -1][ag->posy +1].habitant == NULL)){
			direction(partie, ag, -1, 1);
		}
		else{
			if(partie->plateau[ag->posx][ag->posy].habitant->vsuiv != NULL){
				supprimer_voisin(ag, partie);
				
			}
			else{
				partie->plateau[ag->posx][ag->posy].habitant= NULL;
			}
			ajoute_voisin(partie, ag, ag->posx -1, ag->posy +1);
			ag->posx = (ag->posx) -1;
			ag->posy = (ag->posy) +1;
		}
	}
	else if(ag->posx != ag->destx && ag->posx < ag->destx){
		if(detecte_ennemi(*partie, *ag, 1, 0) == 0 || ((detecte_ennemi(*partie, *ag, 1, 0) == -2 || detecte_ennemi(*partie, *ag, 1, 0) == 2) && partie->plateau[ag->posx +1][ag->posy].habitant == NULL)){
			direction(partie, ag, 1, 0);
		}
		else{
			if(partie->plateau[ag->posx][ag->posy].habitant->vsuiv != NULL){
				supprimer_voisin(ag, partie);
			}
			else{
				partie->plateau[ag->posx][ag->posy].habitant = NULL;
			}
			ajoute_voisin(partie, ag, ag->posx +1, ag->posy);
			ag->posx = (ag->posx) +1;
			ag->posy = (ag->posy);
		}
	}
	else if(ag->posx != ag->destx && ag->posx > ag->destx){
		if(detecte_ennemi(*partie, *ag, -1, 0) == 0  || ((detecte_ennemi(*partie, *ag, -1, 0) == -2 || detecte_ennemi(*partie, *ag, -1, 0) == 2) && partie->plateau[ag->posx -1][ag->posy].habitant == NULL)){
			direction(partie, ag, -1, 0);
		}
		else{
			if(partie->plateau[ag->posx][ag->posy].habitant->vsuiv != NULL){
				supprimer_voisin(ag, partie);
			}
			else{
				partie->plateau[ag->posx][ag->posy].habitant = NULL;
			}
			ajoute_voisin(partie, ag, ag->posx -1, ag->posy);
			ag->posx = (ag->posx) -1;
			ag->posy = (ag->posy);
		}
	}
	else if(ag->posy != ag->desty && ag->posy > ag->desty){
		if(detecte_ennemi(*partie, *ag, 0, -1) == 0  || ((detecte_ennemi(*partie, *ag, 0, -1) == -2 || detecte_ennemi(*partie, *ag, 0, -1) == 2) && partie->plateau[ag->posx][ag->posy-1].habitant == NULL)){
			direction(partie, ag, 0, -1);
		}
		else{
			if(partie->plateau[ag->posx][ag->posy].habitant->vsuiv != NULL){
				supprimer_voisin(ag, partie);
			}
			else{
				partie->plateau[ag->posx][ag->posy].habitant = NULL;
			}
			ajoute_voisin(partie, ag, ag->posx, ag->posy -1);
			ag->posx = (ag->posx);
			ag->posy = (ag->posy) -1;
		}
	}
	else if(ag->posy != ag->desty && ag->posy < ag->desty){
		if(detecte_ennemi(*partie, *ag, 0, 1) == 0  || ((detecte_ennemi(*partie, *ag, 0, 1) == -2 || detecte_ennemi(*partie, *ag, 0, 1) == 2) && partie->plateau[ag->posx][ag->posy+1].habitant == NULL)){
			direction(partie, ag, 0, 1);
		}
		else{
			if(partie->plateau[ag->posx][ag->posy].habitant->vsuiv != NULL){
				supprimer_voisin(ag, partie);
			}
			else{
				partie->plateau[ag->posx][ag->posy].habitant = NULL;
			}
			ajoute_voisin(partie, ag, ag->posx, ag->posy +1);
			ag->posx = (ag->posx);
			ag->posy = (ag->posy) +1;
		}
	}
}

int selectionne_cases(int * x, int * y){
	// Retour : 0 si le clic est dans la grille ou sur un bouton, 1 sinon.
	// (x, y) valent les coordonnées du clic s'ils sont sur la grille
	// sinon x vaut :
	// -1 pour le premier bouton, -2 pour le deuxième, ect. Il y a 5 boutons.

	MLV_wait_mouse(x, y);
	*x = *x / (TAILLE_C);
	*y = *y / (TAILLE_C);

	if (*x < LONG && *y < HAUT) {
		return 0;
	}
	if (*x >= LONG + 1 && *x <= LONG + 3) {
		if (*y == 3) {
			*x = -1;
			return 0;
		}
		if (*y == 5) {
			*x = -2;
			return 0;
		}
		if (*y == 7) {
			*x = -3;
			return 0;
		}
		if (*y == 9) {
			*x = -4;
			return 0;
		}
		if (*y == 11) {
			*x = -5;
			return 0;
		}
		return 1;
	}

	return 1;
}

AListe extraire_agent(AListe ag, Monde *partie){
	//Cette fonction extrait un agent de sa liste, elle prend en argument un Monde et un agent et renvoie l'adresse de ce dernier
	
	AListe tmp;
	tmp = ag;
	//Si l'agent est le dernier de sa liste alors le suivant de son précédant devient NULL et on renvoie l'adresse de l'agent
	if(tmp->asuiv == NULL){
		tmp-> aprec -> asuiv = NULL;
		tmp-> aprec = NULL;
		return tmp;
	}
	else{
		//Sinon l'agent n'est pas le dernier de sa liste
		tmp->aprec->asuiv = tmp->asuiv;
		tmp->asuiv->aprec = tmp->aprec;
		tmp->asuiv = NULL;
		tmp->aprec = NULL;
		return tmp;
	}
}

AListe ajout_chateau(Monde *partie, AListe ag){
	//Cette fonction ajoute un chateau dans la partie, elle prend en argument un Monde, et un agent
	
	//On initialise plusieurs listes, chateau qui pointera sur le chateau créé, tmp une temporaire, et ptmp, le précédant de cette tmp
	AListe chateau;
	AListe tmp;
	AListe ptmp;
	
	//Selon la couleur on place tmp sur telle ou telle équipe
	if(ag -> couleur == 'R') tmp = partie -> rouge;
	else tmp = partie -> bleu;
	
	//On alloue un chateau
	chateau = alloue_agent(ag -> couleur, 'c');
	//Les coordonnées de ce chateaux sont celles de l'agent qui a crée ce chateau
	chateau -> posx = ag->posx;
	chateau -> posy = ag->posy;
	ptmp = tmp;
	while(tmp -> vsuiv != NULL){
		ptmp = tmp;
		tmp = tmp -> vsuiv;
	} 
	//On place ce chateau apres le dernier chateau de la liste
	tmp -> vsuiv = chateau;
	//On définit le vprec du chateau créé
	ptmp = tmp-> vsuiv;
	ptmp -> vprec = tmp;
	//La case pointe sur un chateau
	partie -> plateau[ag->posx][ag->posy].chateau = chateau;
	//On extrait l'agent de sa liste
	ag = extraire_agent(ag,partie);
	//Cet agent devient le premier agent de son chateau
	chateau -> asuiv = ag;
	//On redéfinit le précédant de cet agent
	ag->aprec = chateau;
	//On renvoie l'adresse de l'agent
	return ag;
}

void dessine_selection(Agent ag) {
	if (ag.genre == 'c') {
		MLV_draw_circle(
			ag.posx * TAILLE_C + TAILLE_C / 4, 
			ag.posy * TAILLE_C + TAILLE_C / 4, 
			TAILLE_C / 4, MLV_COLOR_WHITE);
	}
	else if (ag.genre == 'b') {
		MLV_draw_circle(
			ag.posx * TAILLE_C + 3 * TAILLE_C / 4, 
			ag.posy * TAILLE_C + TAILLE_C / 4, 
			TAILLE_C / 4, MLV_COLOR_WHITE);
	}
	else if (ag.genre == 'g') {
		MLV_draw_circle(
			ag.posx * TAILLE_C + TAILLE_C / 4, 
			ag.posy * TAILLE_C + 3 * TAILLE_C / 4, 
			TAILLE_C / 4, MLV_COLOR_WHITE);
	}
	else {
		MLV_draw_circle(
			ag.posx * TAILLE_C + 3 * TAILLE_C / 4, 
			ag.posy * TAILLE_C + 3 * TAILLE_C / 4, 
			TAILLE_C / 4, MLV_COLOR_WHITE);
	}
}

void donne_ordre(Monde * partie, char couleur) {
	// Cette fonction gère les ordre donnés à tout les agents d'une equipe. Elle prends une partie et la couleur à faire jouer en argument
	AListe tmp;				// Une variable represenant l'agent courant.
	AListe tmpc;			// Une variable represenant le chateau courant.
	AListe ag = NULL;
	int err;
	int x;
	int y;
	int tresor;				// Une variable representant le tresor de l'equipe qui joue.
	int extract = 0;		// Une variable permettant de savoir si l'agent est a detruire, afin de gerer l'incrementation de la boucle. 
	int fin = 0;			// Une variable permattant de savoir si o doit terminer le tour sans parcourir toutes les unités.

	// On initialise la variable temporaire en fonction de la couleur.
	// On initialise egalement le trsor, ce qui evitera de tester la couleur à chaque fois qu'il faudra le modifier.
	if (couleur == 'R') {
		tmp = partie -> rouge;
		tresor = partie -> tresorRouge;
	}
	else {
		tmp = partie -> bleu;
		tresor = partie -> tresorBleu;
	}
	tmpc = tmp;

	// On considère qu'il existe forcement un chateau dans chaque equipe, car sinon elle a perdu
	// On parcourt la liste des chateaux
	while(tmpc != NULL && fin == 0){
		// On parcourt les agents liés au chateau courant.
		while (tmp != NULL && tmp != ag && fin == 0) {
			// On teste si l'agent courant doit recevoir un ordre.
			if (test_ordre(*tmp)) {
				// Si oui, on affiche le tour et le tresor de l'equipe.
				if (couleur == 'R') {
					MLV_draw_text_box(
						(LONG + 1) * TAILLE_C,
						TAILLE_C,
						3 * TAILLE_C,
						TAILLE_C,
						"Tour %d\nEquipe Rouge\nTresorerie : %d",
						TAILLE_C / 20,
						MLV_COLOR_WHITE,
						MLV_COLOR_WHITE,
						MLV_COLOR_RED,
						MLV_TEXT_CENTER,
						MLV_HORIZONTAL_CENTER,
						MLV_VERTICAL_CENTER,
						partie -> tour,
						partie -> tresorRouge
					);
				}
				else {
					MLV_draw_text_box(
						(LONG + 1) * TAILLE_C,
						TAILLE_C,
					    3 * TAILLE_C,
						TAILLE_C,
						"Tour %d\nEquipe Bleue\nTresorerie : %d",
						TAILLE_C / 20,
						MLV_COLOR_WHITE,
						MLV_COLOR_WHITE,
						MLV_COLOR_BLUE,
						MLV_TEXT_CENTER,
						MLV_HORIZONTAL_CENTER,
						MLV_VERTICAL_CENTER,
						partie -> tour,
						partie -> tresorBleu
					);
				}
				MLV_actualise_window();
				printf("Ordre de l'agent en (%2d, %2d)\n", tmp -> posx, tmp -> posy);
				// Ensuite on affiche l'agent et les boutons en fonction de son genre.
				affiche_agent(*tmp);
				affiche_bouton(tmp -> genre);
				dessine_selection(*tmp);
				MLV_actualise_window();
				// Si l'agent n'est pas un chateau, on attends un clic de l'utilisateur
				if (tmp -> genre != 'c') {
					do {
						err = selectionne_cases(&x, &y);
						// Tant que l'utilisateur n'a pas séléctionné de case ou de bouton, on attends.
					} while ((err == 1));

					if (x == -1) {
						// Si l'utilisateur a cliqué sur le premier bouton "Dispertion" l'agent sera extrait à la fin de la boucle.
						extract = 1;
					}
					if (x == -2) {
						// Si l'utilisateur a cliqué sur le deuxième bouton, on teste le genre de l'agent.
						if (tmp -> genre == 'b'){
							// Si c'est un baron, le bouton est "Construire un chateau". On teste donc si l'equipe peut construire un chateau
							if(partie->plateau[tmp->posx][tmp->posy].chateau == NULL) {
								if (tresor >= CCHATEAU) {
									// Si oui, on change la destination du baron à (-1, -1). Ainsi, il construira un chateau lors de l'exection des ordres
									ag = ajout_chateau(partie, tmp);
									tresor -= CCHATEAU;
								}
							}
						}
					}
					else if (x == -4) {
						// Si l'utilisateur a cliqué sur le quatrième bouton, "Ne rien faire" on teste le genre de l'agent.
						if (tmp -> genre == 'm'){
							// Si c'est un manant, il souhaite recolter et on teste si c'est possible.
							if (!teste_manant(*partie, *tmp)) {
								// Si oui, on met sa destination à (-1, -1), ainsi lors de l'execution des ordres il fera gagner de l'or à son equipe.
								tmp -> destx = -1;
								tmp -> desty = -1;
							}
						}
						else {
							// Si l'agent n'est pas un manant, il passe son tour.
							tmp -> destx = tmp -> posx;
							tmp -> desty = tmp -> posy;
						}
					}
					else if (x == -5) {
						// Si l'utilisateur a cliqué sur le cinquième bouton, il souhaite passer le tour en entier. On met la variable de fin à 1.
						fin = 1;
					}
					else {
						// Si l'utilisateur n'as pas cliqué sur un bouton, il a cliqué sur la carte, et l'agent a donc pour destination le clic.
						tmp -> destx = x;
						tmp -> desty = y;
					}

				}
				else {
					// Pour un chateau, on demande egalement un clic.
					do {
						err = selectionne_cases(&x, &y);
					} while ((err == 1) || x >= 0);
					if (x == -1) {
						// Si l'utilisateur a cliqué sur un des 3 remier boutons, il souhaite construire une unité.
						// On verifie que son budget le lui permet.
						if (tresor >= CMANANT) {
							// Si oui on modifie la production, le temps restant, et le tresor en consequence.
							tmp -> produit = 'm';
							tmp -> temps = TMANANT;
							tresor -= CMANANT;
						}
						else {
							// Sinon, on met la production à '0' pour indiquer que le chateau ne produit rien.		
							tmp -> produit = '0';
							tmp -> temps = 1;	
						}	
					}
					else if (x == -2) {
						if (tresor >= CGUERRIER) {
							tmp -> produit = 'g';
							tmp -> temps = TGUERRIER;
							tresor -= CGUERRIER;
						}
						else {					
							tmp -> produit = '0';
							tmp -> temps = 1;	
						}

					}
					else if (x == -3) {
						if (tresor >= CBARON) {
							tmp -> produit = 'b';
							tmp -> temps = TBARON;
							tresor -= CBARON;
						}
						else {					
							tmp -> produit = '0';
							tmp -> temps = 1;	
						}
					}
					else if (x == -5) {
						// Si l'utilisateur a cliqué sur le cinquième bouton, il souhaite passer le tour en entier. On met la variable de fin à 1.
						fin = 1;
					}
					else {						// On met la production à 0 afin de montrer que le chateau ne produit rien.
						tmp -> produit = '0';	// On met le temps à 1 car lors de l'execution des ordres, le temps decrementera de 1 et reviendra à 0.
						tmp -> temps = 1;		// Ainsi le chateau demandera un nouvel ordre au tour suivant.
					}
				}
			}
			if (couleur == 'R') partie -> tresorRouge = tresor;
			else partie -> tresorBleu = tresor;
			// On met ensuite à jour le tresor de l'equipe grace à la variable tresor.

			if (extract) {
				// Si l'agent doit etre detruit, on l'extrait de la liste
				ag = tmp -> asuiv;
				tmp = extraire_agent(tmp, partie);
				// On le supprime de la liste des voisins.
				if (tmp -> vsuiv == NULL && tmp -> vprec == NULL) {
					partie -> plateau[tmp -> posx][tmp -> posy].habitant = NULL;
				}
				else { 
					supprimer_voisin(tmp, partie);
				}
				// Puis on libère l'espace utilisé.
				free(tmp);
				// Enfin on remet la variable à 0.
				extract = 0;
				tmp = ag;
			}
			else tmp = tmp -> asuiv;
			// On fait progresser l'agent courant, puis on affiche le contenu de la carte.
			affiche_contenu(*partie);
		}
		// On fait progresser le chateau courant et on met l'agent courant au debut de la liste des agents liés à ce chateau.
		tmpc = tmpc -> vsuiv;
		tmp = tmpc;
	}
}

void execute_ordre(Monde * partie, char couleur) {
	// Cette fonction execute les ordres donnés lors de donne_ordre. Elle prends les mêmes arguments.
	AListe lst_agent = NULL;
	AListe tmp;
	AListe tmpc;
	// on utilise les mêmes temporaires que pour donne_ordre
	// AListe ag = NULL;

	if (couleur == 'R') tmp = partie -> rouge;
	else tmp = partie -> bleu;
	// On initialise les temporaires en fonction de la couleur.
	tmpc = tmp;

	// On parcourt la liste des chateaux.
	while (tmpc != NULL){
		// On parcourt la liste des agent du chateau.s
		while (tmp != NULL) {
			if (tmp -> genre != 'c') {
				// Si l'agent n'est pas un chateau, on teste son genre.
				if (tmp -> genre == 'g') {
					// Si c'est un guerrier, son seul ordre peut etre de se deplacer.
					deplacement_agent(partie, tmp);	
				}
				else if (tmp -> genre == 'm') {
					// Si c'est un manant, on teste sa destination.
					if (tmp -> destx >= 0 && tmp -> desty >= 0) {
						// Si elle est positive, on le deplace.
						deplacement_agent(partie, tmp);
					}
					else {
						// Sinon on augmente le tresor car le paysan recolte.
						if (couleur == 'R') partie -> tresorRouge++;
						else partie -> tresorBleu++;
					}
				}
				else {
					// Si c'est un baron, on teste de même sa destination.
					if (tmp -> destx >= 0 && tmp -> desty >= 0) {
						// Si elle est positive, on le deplace.
						deplacement_agent(partie, tmp);
					}
					// else {
					// 	// Sinon on ajoute un chateau.
					// 	ag = ajout_chateau(partie, tmp);
					// 	tmp -> destx = tmp -> posx;
					// 	tmp -> desty = tmp -> posy;
					// }	
				}
			}
			else {
				// Si c'est un chateau, on teste le temps restant pour sa production.
				if (tmp -> temps > 0) {
					// S'il reste du temps, on le fait baisser.
					tmp -> temps--;
				}
				// On teste ensuite si le temps est à 0 et que le chateau ait a produire quelque chose.
				if (tmp -> produit != '0' && tmp -> temps == 0) {
					// Si oui, on place l'agent que l'on vient de creer.
					place_agent_cree(partie, tmp -> produit, tmp);
				}
			}
			tmp = tmp -> asuiv;
			// On fait progresser les temporaires.
		}
		tmpc = tmpc -> vsuiv;
		tmp = tmpc;
	}
	//Une fois que tous les agent se sont déplacés, on peut résoudre les combats
	if (couleur == 'R') tmp = partie -> rouge;
	else tmp = partie -> bleu;
	tmpc = tmp;
	//On reparcourt donc la liste des agents
	while (tmpc != NULL){
		while (tmp != NULL) {
			if(tmp->genre != 'c'){
				//Si un agent va sur une case où il y a un chateau ennemi sans garnison, le chateau est detruit donc on le supprime.
				if(partie->plateau[tmp->posx][tmp->posy].habitant-> couleur == tmp-> couleur && partie->plateau[tmp->posx][tmp->posy].chateau != NULL && partie->plateau[tmp->posx][tmp->posy].chateau->couleur != tmp->couleur){
					lst_agent = supprime_chateau(partie, partie->plateau[tmp->posx][tmp->posy].chateau);
					partie->plateau[tmp->posx][tmp->posy].chateau = NULL;
					if(lst_agent != NULL){
						//Si le chateau possède des agents on peut rattacher cette liste.
						rattache_agent(partie, lst_agent, partie->plateau[tmp->posx][tmp->posy].habitant);
					}
				}
				//Sinon si un l'agent surlequel la case pointe n'est pas de sa propre équipe alors un combat s'engage
				else if(tmp->couleur != partie->plateau[tmp->posx][tmp->posy].habitant->couleur){
					combat(partie, tmp->posx, tmp->posy);
				}
			}
			tmp = tmp -> asuiv;
		}
		tmpc = tmpc -> vsuiv;
		tmp = tmpc;
	}
}

int ecran_debut_tour(char couleur, int sauvegarde) {
	// Cette fonction affiche l'ecran de debut de tour.
	int x;
	int x_gauche = (TAILLE_X - 5 * TAILLE_C) / 2;
	int x_droite = x_gauche + 5 * TAILLE_C;
	int y;
	int y_tab[6] = {(TAILLE_Y - 5 * TAILLE_C) / 2,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 2 * TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 3 * TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 4 * TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 5 * TAILLE_C
	};

	// On utilise un certain nombre de variables pour simplifier les coordonées.
	// On affiche differents boutons en fonction de saubegarde, qui determine si on doit afficher ou non le bouton de sauvegarde.

	MLV_clear_window(MLV_COLOR_BLACK);
	if (couleur == 'R') {
		if (sauvegarde) {
			MLV_draw_text_box(
				x_gauche,
				y_tab[0],
				5 * TAILLE_C,
				TAILLE_C,
				"Tour du joueur rouge",
				TAILLE_C / 20,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_RED,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
			MLV_draw_text_box(
				x_gauche,
				y_tab[2],
				5 * TAILLE_C,
				TAILLE_C,
				"Jouer",
				TAILLE_C / 20,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_BLACK,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
			MLV_draw_text_box(
				x_gauche,
				y_tab[4],
				5 * TAILLE_C,
				TAILLE_C,
				"Sauvegarder",
				TAILLE_C / 20,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_BLACK,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
			MLV_actualise_window();
			do {
				MLV_wait_mouse(&x, &y);
			} while (x < x_gauche || x > x_droite || ((y < y_tab[2] || y > y_tab[3]) && (y < y_tab[4] || y > y_tab[5])));
			if (y >= y_tab[2] && y <= y_tab[3]) return 0;
			else return 1;
		}
		else {
			MLV_draw_text_box(
				x_gauche,
				y_tab[1],
				5 * TAILLE_C,
				TAILLE_C,
				"Tour du joueur rouge",
				TAILLE_C / 20,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_RED,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
			MLV_draw_text_box(
				x_gauche,
				y_tab[3],
				5 * TAILLE_C,
				TAILLE_C,
				"Jouer",
				TAILLE_C / 20,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_BLACK,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
			MLV_actualise_window();
			do {
				MLV_wait_mouse(&x, &y);
			} while (x < x_gauche || x > x_droite || (y < y_tab[3] || y > y_tab[4]));
			return 0;
		}
	}
	else {
		if (sauvegarde) {
			MLV_draw_text_box(
				x_gauche,
				y_tab[0],
				5 * TAILLE_C,
				TAILLE_C,
				"Tour du joueur bleu",
				TAILLE_C / 20,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_BLUE,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
			MLV_draw_text_box(
				x_gauche,
				y_tab[2],
				5 * TAILLE_C,
				TAILLE_C,
				"Jouer",
				TAILLE_C / 20,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_BLACK,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
			MLV_draw_text_box(
				x_gauche,
				y_tab[4],
				5 * TAILLE_C,
				TAILLE_C,
				"Sauvegarder",
				TAILLE_C / 20,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_BLACK,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
			MLV_actualise_window();
			do {
				MLV_wait_mouse(&x, &y);
			} while (x < x_gauche || x > x_droite || ((y < y_tab[2] || y > y_tab[3]) && (y < y_tab[4] || y > y_tab[5])));
			if (y >= y_tab[2] && y <= y_tab[3]) return 0;
			else return 1;
		}
		else {
			MLV_draw_text_box(
				x_gauche,
				y_tab[1],
				5 * TAILLE_C,
				TAILLE_C,
				"Tour du joueur bleu",
				TAILLE_C / 20,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_BLUE,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
			MLV_draw_text_box(
				x_gauche,
				y_tab[3],
				5 * TAILLE_C,
				TAILLE_C,
				"Jouer",
				TAILLE_C / 20,
				MLV_COLOR_WHITE,
				MLV_COLOR_WHITE,
				MLV_COLOR_BLACK,
				MLV_TEXT_CENTER,
				MLV_HORIZONTAL_CENTER,
				MLV_VERTICAL_CENTER
			);
			MLV_actualise_window();
			do {
				MLV_wait_mouse(&x, &y);
			} while (x < x_gauche || x > x_droite || (y < y_tab[3] || y > y_tab[4]));
			return 0;
		}
	}
}

int menu_box(char * nom_fic, char * message) {
	// Cette fonction similaire à ecran_debut_tour affiche un menu contenant deux messages et renvoie un entier en fonction du choix de l'utilisateur :
	//  "annuler" (0) et un autre choisi (1), et une imput_box pour rentrer du texte.
	char * saisie;
	int x;
	int x_gauche = (TAILLE_X - 5 * TAILLE_C) / 2;
	int x_droite = x_gauche + 5 * TAILLE_C;
	int y;
	int y_tab[6] = {(TAILLE_Y - 5 * TAILLE_C) / 2,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 2 * TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 3 * TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 4 * TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 5 * TAILLE_C
	};

	MLV_clear_window(MLV_COLOR_BLACK);
	MLV_draw_text_box(
		x_gauche,
		y_tab[2],
		5 * TAILLE_C,
		TAILLE_C,
		message,
		TAILLE_C / 20,
		MLV_COLOR_WHITE,
		MLV_COLOR_WHITE,
		MLV_COLOR_BLACK,
		MLV_TEXT_CENTER,
		MLV_HORIZONTAL_CENTER,
		MLV_VERTICAL_CENTER
	);
	MLV_draw_text_box(
		x_gauche,
		y_tab[4],
		5 * TAILLE_C,
		TAILLE_C,
		"Annuler",
		TAILLE_C / 20,
		MLV_COLOR_WHITE,
		MLV_COLOR_WHITE,
		MLV_COLOR_BLACK,
		MLV_TEXT_CENTER,
		MLV_HORIZONTAL_CENTER,
		MLV_VERTICAL_CENTER
	);
	MLV_wait_input_box(
		x_gauche,
		y_tab[0],
		5 * TAILLE_C,
		TAILLE_C,
		MLV_COLOR_WHITE,
		MLV_COLOR_WHITE,
		MLV_COLOR_BLACK,
		"Nom du fichier : ",
		&saisie
	);
	strcpy(nom_fic, saisie);
	// On recupère la valeur données par l'input box.
	
	do {
		MLV_wait_mouse(&x, &y);
	} while (x < x_gauche || x > x_droite || ((y < y_tab[2] || y > y_tab[3]) && (y < y_tab[4] || y > y_tab[5])));
	// Puis on attends que l'utilisateur ait cliqué sur un bouton.
	
	if (y >= y_tab[2] && y <= y_tab[3]) return 1;
	else return 0;
}

void sauvegarde(FILE * fichier, Monde partie, char couleur) {
	// Cette fonction permet de sauvegarder une partie dans un fichier passé en argument.
	AListe tmp;
	AListe tmp_c;
	// Deux temporaires permettant de parcourir les chateaux et les agent qui y sont liés.

	// On a ajouté le tour à la sauvegarde afin de charger une partie en connaissant le nombre de tour
	if (couleur == 'R') {
		// On ecrit dans le fichier la couleurdevant jouer, le tresor de cette equipe, uis de l'autre equipe, puis le tour.
		fprintf(fichier, "R %d %d %d\n", partie.tresorRouge, partie.tresorBleu, partie.tour);
	}
	else {
		fprintf(fichier, "B %d %d %d\n", partie.tresorBleu, partie.tresorRouge, partie.tour);
	}

	// On sauvegarde d'abord l'equipe rouge.
	tmp = partie.rouge;
	tmp_c = tmp;
	
	// On parcourt la liste des chateaux rouges.
	while(tmp_c != NULL) {
		tmp = tmp_c;
		// Puis les agents liés au chateau
		while (tmp != NULL) {
			// On sauvegarde dans le fichier la couleur, le genre, la position, la destination, la production, le temps restant de l'agent.
			fprintf(fichier, "R %c %d %d %d %d %d %d\n", tmp -> genre, tmp -> posx, tmp -> posy, tmp -> destx, tmp -> desty, tmp -> produit, tmp -> temps);
			tmp = tmp -> asuiv;
		}
		tmp_c = tmp_c -> vsuiv;
	}

	// On sauvegarde ensuite de même l'equipe bleue.
	tmp = partie.bleu;
	tmp_c = tmp;
	
	while(tmp_c != NULL) {
		tmp = tmp_c;
		while (tmp != NULL) {
			fprintf(fichier, "B %c %d %d %d %d %d %d\n", tmp -> genre, tmp -> posx, tmp -> posy, tmp -> destx, tmp -> desty, tmp -> produit, tmp -> temps);
			tmp = tmp -> asuiv;
		}
		tmp_c = tmp_c -> vsuiv;
	}
}

int menu_principal(){
	// Cette fonction affiche le menu principal et renvoie un entier representant le choix de l'utilisateur.
	int x;
	int x_gauche = (TAILLE_X - 5 * TAILLE_C) / 2;
	int x_droite = x_gauche + 5 * TAILLE_C;
	int y;
	int y_tab[6] = {(TAILLE_Y - 5 * TAILLE_C) / 2,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 2 * TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 3 * TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 4 * TAILLE_C,
		(TAILLE_Y - 5 * TAILLE_C) / 2 + 5 * TAILLE_C
	};

	MLV_clear_window(MLV_COLOR_BLACK);
	MLV_draw_text_box(
		x_gauche,
		y_tab[1],
		5 * TAILLE_C,
		TAILLE_C,
		"Nouvelle partie",
		TAILLE_C / 20,
		MLV_COLOR_WHITE,
		MLV_COLOR_WHITE,
		MLV_COLOR_BLACK,
		MLV_TEXT_CENTER,
		MLV_HORIZONTAL_CENTER,
		MLV_VERTICAL_CENTER
	);
	MLV_draw_text_box(
		x_gauche,
		y_tab[3],
		5 * TAILLE_C,
		TAILLE_C,
		"Charger",
		TAILLE_C / 20,
		MLV_COLOR_WHITE,
		MLV_COLOR_WHITE,
		MLV_COLOR_BLACK,
		MLV_TEXT_CENTER,
		MLV_HORIZONTAL_CENTER,
		MLV_VERTICAL_CENTER
	);
	MLV_actualise_window();

	// On affiche les choix de l'utilisateur puis on attends un clic valide.
	do {
		MLV_wait_mouse(&x, &y);
	} while (x < x_gauche || x > x_droite || ((y < y_tab[1] || y > y_tab[2]) && (y < y_tab[3] || y > y_tab[4])));
	
	// On renvoie ensuite les valeurs correspondantes : 0 pour la nouvelle partie, 1 pour charger.
	if (y >= y_tab[1] && y <= y_tab[2]) return 0;
	else return 1;
}

char charge(FILE * fichier, Monde * partie) {
	// Cette fonction charge une sauvegarde à partir d'un fichier de sauvegarde.
	// Retour : la lettre de la couleur de l'equipe devant jouer.
	// On crée une variable pour chaque attribut, pour ne pas avoir à ecrire directement dans la partie dans les fscanf().
	char couleur = 'R'; 
	char genre = 'c';
	int posx, posy;
	int destx, desty;
	int produit; // Comme indiqué dans le sujet, la production d'un chateau a été sauvegardée en tant qu'entier (2 char et 6 int)
	int temps;
	char joueur;
	int err;
	// on utilise ici trois variables temporaires : 
	AListe tmp_a = NULL;	// l'agent courant
	AListe tmp_p = NULL;	// son precedent
	AListe tmp_c = NULL;	// et le chateau

	// On lit le premier caractère du fichier, qui represente le joueur devant jouer.
	err = fscanf(fichier, "%c", &joueur);

	// On lit ensuite la valeur de chaque tresor ainsi que le tour, en fonction du joueur devant jouer.
	if (joueur == 'R') {
		err = fscanf(fichier, "%d %d %d\n", &(partie -> tresorRouge), &(partie -> tresorBleu), &(partie -> tour));
	}
	else {
		err = fscanf(fichier, "%d %d %d\n", &(partie -> tresorBleu), &(partie -> tresorRouge), &(partie -> tour));
	}

	// Puis on lit chaque ligne, on commence par en lire une avant la boucle.
	err = fscanf(fichier, "%c %c %d %d %d %d %d %d\n", &couleur, &genre, &posx, &posy, &destx, &desty, &produit, &temps);
	// On alloue un nouvel agent, puis on lui ajoute les attibuts qu'on a recupéré plus haut.
	tmp_a = alloue_agent(couleur, genre);
	tmp_a -> posx = posx;
	tmp_a -> posy = posy;
	tmp_a -> destx = destx;
	tmp_a -> desty = desty;
	tmp_a -> produit = produit;
	tmp_a -> temps = temps;

	if (tmp_a -> genre != 'c') {
		// Si l'agent recupéré n'est pas un chateau, on l'ajoute sur sa case.
		if (partie -> plateau[posx][posy].habitant == NULL) {
			// S'il n'y a personne sur cette case, on utilise direction pour l'ajouter.
			direction(partie, tmp_a, 0, 0);
		}
		else {
			// Sinon on l'insère dans la liste chainée avec ajoute_voisin.
			ajoute_voisin(partie, tmp_a, posx, posy);
		}
	}
	// Si c'est un chateau, on change simplement case.chateau.
	else partie -> plateau[posx][posy].chateau = tmp_a;

	// On initialise ensuite les temporaires : 
	// Le precedent vaut pour l'instant tmp_a
	tmp_p = tmp_a;
	// Le chateau aussi, car pour le premier element de la liste tmp_a est un chateau.
	tmp_c = tmp_a;
	// Ensuite on initialise partie -> rouge, car on a commencé par souvegarder les rouges.
	partie -> rouge = tmp_a;
	// Puis on lit la ligne suivante.
	err = fscanf(fichier, "%c %c %d %d %d %d %d %d\n", &couleur, &genre, &posx, &posy, &destx, &desty, &produit, &temps);

	// On avance ensuite dans le fichier, tant que la couleur est rouge. 
	// Il y aura forcement au moins un chateau bleu, sinon les bleus ont perdu.
	while (couleur == 'R') {
		while (genre != 'c'){
			// Après avoir trouvé un chateau, on avance et on ajoute les unités une par une à ce chateau.
			tmp_a = alloue_agent(couleur, genre);
			// Comme a chaque fois, on alloue puis ajoute ses attributs à un agent.
			tmp_a -> posx = posx;
			tmp_a -> posy = posy;
			tmp_a -> destx = destx;
			tmp_a -> desty = desty;
			tmp_a -> produit = produit;
			tmp_a -> temps = temps;
			// On utilise la même technique pour ajouter l'agent à sa case.
			if (tmp_a -> genre != 'c') {
				if (partie -> plateau[posx][posy].habitant == NULL) {
					direction(partie, tmp_a, 0, 0);
				}
				else {
					ajoute_voisin(partie, tmp_a, posx, posy);
				}
			}
			else partie -> plateau[posx][posy].chateau = tmp_a;
			// Puis on utilise tmp_p pour lettre indiquer à chaque agent son suivant/precedent.
			tmp_p -> asuiv = tmp_a;
			tmp_a -> aprec = tmp_p;
			tmp_p = tmp_a;
			// puis on lit la ligne suivante
			err = fscanf(fichier, "%c %c %d %d %d %d %d %d\n", &couleur, &genre, &posx, &posy, &destx, &desty, &produit, &temps);
		}
		// Avant de passer au cas où tmp_a est un chateau on verifie qu'il est toujours rouge.
		if (couleur == 'R') {
			// Ensuite on l'alloue, et lui ajoute ses attributs.
			tmp_a = alloue_agent(couleur, genre);
			tmp_a -> posx = posx;
			tmp_a -> posy = posy;
			tmp_a -> destx = destx;
			tmp_a -> desty = desty;
			tmp_a -> produit = produit;
			tmp_a -> temps = temps;
			// Puis on le place sur sa case.
			if (tmp_a -> genre != 'c') {
				if (partie -> plateau[posx][posy].habitant == NULL) {
					direction(partie, tmp_a, 0, 0);
				}
				else {
					ajoute_voisin(partie, tmp_a, posx, posy);
				}
			}
			else partie -> plateau[posx][posy].chateau = tmp_a;

			// Ensuite on change ses "voisins", utilisés pour la liste des chateaux
			if (tmp_c != NULL) {
				tmp_c -> vsuiv = tmp_a;
				tmp_a -> vprec = tmp_c;
			}
			// Puis on fait avancer tmp_p et tmp_c.
			tmp_p = tmp_a;
			tmp_c = tmp_a;

			err = fscanf(fichier, "%c %c %d %d %d %d %d %d\n", &couleur, &genre, &posx, &posy, &destx, &desty, &produit, &temps);
		}
	}

	// Ensuite on réinitialise les trois temporaires, pour passer à la deuxième equipe.
	tmp_p = NULL;
	tmp_a = NULL;
	tmp_c = NULL;

	// On recommence pareil que pour les rouges.
	tmp_a = alloue_agent(couleur, genre);
	tmp_a -> posx = posx;
	tmp_a -> posy = posy;
	tmp_a -> destx = destx;
	tmp_a -> desty = desty;
	tmp_a -> produit = produit;
	tmp_a -> temps = temps;
	if (tmp_a -> genre != 'c') {
		if (partie -> plateau[posx][posy].habitant == NULL) {
			direction(partie, tmp_a, 0, 0);
		}
		else {
			ajoute_voisin(partie, tmp_a, posx, posy);
		}
	}
	else partie -> plateau[posx][posy].chateau = tmp_a;

	tmp_p = tmp_a;
	partie -> bleu = tmp_a;
	tmp_c = tmp_a;

	err = fscanf(fichier, "%c %c %d %d %d %d %d %d\n", &couleur, &genre, &posx, &posy, &destx, &desty, &produit, &temps);

	// Sauf qu'au lieu de verifier la couleur, on vérifie qu'on est pas à la fin du fichier.
	while (err != EOF) {
		while (genre != 'c' && err != EOF){
			tmp_a = alloue_agent(couleur, genre);
			tmp_a -> posx = posx;
			tmp_a -> posy = posy;
			tmp_a -> destx = destx;
			tmp_a -> desty = desty;
			tmp_a -> produit = produit;
			tmp_a -> temps = temps;
			if (tmp_a -> genre != 'c') {
				if (partie -> plateau[posx][posy].habitant == NULL) {
					direction(partie, tmp_a, 0, 0);
				}
				else {
					ajoute_voisin(partie, tmp_a, posx, posy);
				}
			}
			else partie -> plateau[posx][posy].chateau = tmp_a;

			tmp_p -> asuiv = tmp_a;
			tmp_a -> aprec = tmp_p;
			tmp_p = tmp_a;
			err = fscanf(fichier, "%c %c %d %d %d %d %d %d\n", &couleur, &genre, &posx, &posy, &destx, &desty, &produit, &temps);
		}

		if (err != EOF) {
			tmp_a = alloue_agent(couleur, genre);
			tmp_a -> posx = posx;
			tmp_a -> posy = posy;
			tmp_a -> destx = destx;
			tmp_a -> desty = desty;
			tmp_a -> produit = produit;
			tmp_a -> temps = temps;
			if (tmp_a -> genre != 'c') {
				if (partie -> plateau[posx][posy].habitant == NULL) {
					direction(partie, tmp_a, 0, 0);
				}
				else {
					ajoute_voisin(partie, tmp_a, posx, posy);
				}
			}
			else partie -> plateau[posx][posy].chateau = tmp_a;

			if (tmp_c != NULL) {
				tmp_c -> vsuiv = tmp_a;
				tmp_a -> vprec = tmp_c;
			}
			tmp_p = tmp_a;
			tmp_c = tmp_a;

			err = fscanf(fichier, "%c %c %d %d %d %d %d %d\n", &couleur, &genre, &posx, &posy, &destx, &desty, &produit, &temps);
		}
	}

	// Enfin on renvoie la couleur qui doit jouer en premier.
	return joueur;
}

int main(void){
	Monde partie;			// La variable qui permet de jouer
	int clic = 0;			// Une variable qui récupère la valeur du clic de l'utilisateur.
	int err = 0;			// Une variable permettant de recuperer un cod d'erreur.
	char joueur;			// Un caractère qui determine quel joueur jouera en premier.
	char fic[20];			// Le nom du fichier pour la sauvegarde et le chargement.
	FILE * fichier;			// Le fichier en lui même.
	
	// On crée la fenetre MLV
	MLV_create_window("Age des Barons", "Age des Barons", TAILLE_X, TAILLE_Y);
	// On initialise l'aleatoire.
	srand(time(NULL));

	do {
		// On affiche ensuite le menu principal, et on attends que l'uilisatuer ait cliqué sur un des deux boutons "Nouvelles partie" ou "Charger"
		clic = menu_principal();

		if (!clic) {
			// S'il a cliqué sur "Nouvelle partie", on initialise le monde, et la couleur du joueur.
			init_monde(&partie);
			err = 0;
			if (rand() % 2 == 0) joueur = 'R';
			else joueur = 'B';
		}
		else {
			// Sinon on affiche le menu de chargement, et on attends que l'utilisateur clic sur un bouton "Charger" ou "Annuler".
			clic = menu_box(fic, "Charger");
			if (clic) {
				// S'il a cliqué sur "Charger", on ouvre le fichier.
				fichier = fopen(fic, "r");
				if (fichier == NULL) {
					// Si le fichier ne s'est pas ouvert correctement, on affiche une erreur.
					err = 1;
					printf("Ce fichier n'existe pas ou est illisible.\n");
				}
				else {
					// Sinon on l'ouvre, on initialise le plateau puis on charge la partie.
					err = 0;
					init_plateau(&partie);
					joueur = charge(fichier, &partie);
					fclose(fichier);
					// Ensuite on ferme le fichier.
				}
			}
			else {
				// S'il a cliqué sur annuler, on revient au menu principal.
				err = 1;
			}
		}
	} while (err);

	// Une fois le monde chargé ou initialsé, on commence la boucle et on s'arrete que l'une des deux equipe est NULL.
	while(partie.bleu != NULL && partie.rouge != NULL) {
		// Si le joueur rouge joue en premier on commence par l'equipe rouge.
		if (joueur == 'R') {
			// On affiche l'ecran de debut de tour et on attends un clic de l'utilisateur
			clic = ecran_debut_tour('R', 1);
			if (clic) {
				// S'il a cliqué sur "Sauvegarder", on affiche le menu de sauvegarde.
				clic = menu_box(fic, "Sauvegarder");
				if (clic) {
					// S'il a cliqué sur "Sauvegarder" on ouvre le fichier, et on sauvegarde dedans la partie.
					fichier = fopen(fic, "w");
					sauvegarde(fichier, partie, 'R');
					fclose(fichier);
					// Puis on ferme le fichier.
				}
			}
			affiche_contenu(partie);
			// On affiche la partie dans la fenetre graphique et le tour dans la console.
			printf("Tour %d\n", partie.tour);

			// On affiche la liste des agents de l'equipe rouge
			printf("Equipe rouge\n");
			affiche_liste_equipe(partie.rouge);
			
			// Puis on demande les ordres de chaque agent.
			donne_ordre(&partie, 'R');

			if(partie.bleu != NULL){
				// Ensuite ou affiche le debut du tour du deuxième joueur, sans lui proposer de sauvgarder.
				clic = ecran_debut_tour('B', 0);
				// On affiche le contenu, et la liste des agents de l'equipe.
				affiche_contenu(partie);
				printf("Equipe bleue\n");
				affiche_liste_equipe(partie.bleu);

				// Puis on demande les ordres de chaque agent.
				donne_ordre(&partie, 'B');

				// Enfin, on execute les ordres des agents des deux equipes.
				execute_ordre(&partie, 'R');
				execute_ordre(&partie, 'B');
				partie.tour ++;
			}
		}
		else {
			// Si le joueur devant jouer est le bleu, on fait la même chose mais en faisant jouer le bleu d'abord.
			clic = ecran_debut_tour('B', 1);
			if (clic) {
				clic = menu_box(fic, "Sauvegarder");
				if (!clic) {
					printf("sauvegarde\n");
				}
			}
			affiche_contenu(partie);
			printf("Tour %d\n", partie.tour);

			printf("Equipe bleue\n");
			affiche_liste_equipe(partie.bleu);
			donne_ordre(&partie, 'B');

			if(partie.rouge != NULL){
				clic = ecran_debut_tour('R', 0);
				affiche_contenu(partie);
				printf("Equipe rouge\n");
				affiche_liste_equipe(partie.rouge);
				donne_ordre(&partie, 'R');

				execute_ordre(&partie, 'B');
				execute_ordre(&partie, 'R');
				partie.tour ++;
			}
		}
		// A la find e la boucle on tire au sort le joueur suivant.
		if (rand() % 2 == 0) joueur = 'R';
		else joueur = 'B';
	}

	// Après la boucle, on teste l'equipe qui a perdu pour afficher un ecran de victoire à son adversaire.
	if(partie.bleu == NULL){
		MLV_clear_window(MLV_COLOR_BLACK);
		MLV_draw_text_box(
			(TAILLE_X - 3 * TAILLE_C) / 2,
			(TAILLE_Y - TAILLE_C) / 2,
			3 * TAILLE_C,
			TAILLE_C,
			"VICTOIRE",
			TAILLE_C / 20,
			MLV_COLOR_WHITE,
			MLV_COLOR_WHITE,
			MLV_COLOR_RED,
			MLV_TEXT_CENTER,
			MLV_HORIZONTAL_CENTER,
			MLV_VERTICAL_CENTER);
	}
	else {
		MLV_clear_window(MLV_COLOR_BLACK);
		MLV_draw_text_box(
			(TAILLE_X - 3 * TAILLE_C) / 2,
			(TAILLE_Y - TAILLE_C) / 2,
			3 * TAILLE_C,
			TAILLE_C,
			"VICTOIRE",
			TAILLE_C / 20,
			MLV_COLOR_WHITE,
			MLV_COLOR_WHITE,
			MLV_COLOR_BLUE,
			MLV_TEXT_CENTER,
			MLV_HORIZONTAL_CENTER,
			MLV_VERTICAL_CENTER);
	}
	MLV_actualise_window();
	MLV_wait_mouse(NULL, NULL);
	// Puis, après un clic de l'utilisateur, on quitte la fenetre et le programme.
	MLV_free_window();
	return 0;
}
