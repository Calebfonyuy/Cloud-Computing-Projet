# Cloud-Computing-Projet
Ce dépot présente l'outil de monitoring que nous avons développé pour les disques de machines virtuelles au format QCOW2. Ce format de disque est utilisé par KVM/QEMU pour le "stockage distribué" des données  sur des éléments appelés clusters dont la taille est fixé à 64ko par convention. Ainsi donc étant donné une chaine de snapshots crée par l'utilisateur, pour chaque cluster actif sur le disque notre outil permet de :
- Donner la profondeur de chacun de ces clusters dans la chaine de snapshots
-  Donner l'intensité des lecture et écriture pour chacun de ces clusters

Le code fournit présente une petite démo de cet outil (c'est un serveur nodejs qui dessert les pages présentants des graphiques et tableaux pour les éléments de performance cités plus haut).  ainsi que la version de notre QEMU modifié.

Les langages utilisés sont le C,python, Javascript et un peu de script bash
