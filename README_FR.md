# TEK IT — Projet VST3 Windows

Projet source C++ / JUCE / CMake pour un synthétiseur VST3 destiné à FL Studio.
Identité visuelle : bleu nuit, bleu électrique et violet fluorescent.

## Ce qui est inclus
- Projet CMake avec JUCE 8.0.6 récupéré automatiquement au build.
- Workflow GitHub Actions Windows x64 qui compile le VST3 et publie un artefact.
- 30 programmes : 10 Metallic Leads, 10 SuperSaw Leads, 10 Hybrid Synths.
- Paramètres exposés à l'hôte : Wave, Detune, Unison, Sub, Cutoff, Resonance, Drive,
  ADSR, Pitch, Fine, EQ Low/Mid/High, Reverb, Delay, Delay Time, Width et Master.
- Génération sonore par oscillateurs, filtre, enveloppe, saturation et réverbération.

## Important — première version de travail
Ce ZIP est une base de développement compilable à valider, pas un binaire déjà testé dans FL Studio.
La compilation GitHub nécessite un accès réseau pour télécharger JUCE. Après le build, il faudra
tester l'artefact dans FL Studio. Les valeurs EQ et Delay sont exposées pour l'automation mais
leur traitement audio complet reste à finaliser. Les 30 programmes sont des variations synthétiques
initiales ; les trois morceaux WAV de référence ne sont pas incorporés dans le ZIP et doivent
servir à affiner les presets à l'étape suivante.

## Compilation via GitHub
1. Crée un nouveau dépôt GitHub nommé `TEK-IT`.
2. Décompresse le ZIP puis envoie **le contenu** du dossier dans le dépôt (CMakeLists.txt doit être à la racine).
3. Ouvre l'onglet **Actions**, active les workflows si GitHub le demande.
4. Lance **Build TEK IT VST3 (Windows x64)** avec **Run workflow**.
5. Quand le workflow termine, ouvre son exécution et télécharge l'artefact `TEK-IT-VST3-Windows-x64`.
6. Décompresse l'artefact. Copie `TEK IT.vst3` dans un dossier VST3 scanné par FL Studio, par exemple
   `C:\\Program Files\\Common Files\\VST3\\`.
7. Dans FL Studio, lance un rescan des plugins et charge TEK IT.

## Si le build échoue
Ouvre l'exécution GitHub Actions, copie le premier message d'erreur rouge complet et envoie-le.
Évite de modifier plusieurs fichiers au hasard : on corrigera à partir de la première vraie erreur.
