# TEK IT — version de travail REWORK V3

Synthétiseur VST3 JUCE pour FL Studio, ciblant les leads tekstyle, jumpstyle et hard dance.

## Modifications de cette version
- Interface redimensionnable avec trois sections et commandes rotary explicitement visibles, noms et valeurs.
- 26 paramètres exposés dans l'interface via APVTS/SliderAttachment pour l'automation hôte.
- Ajout d'un second oscillateur avec mix, octave, FM et composante bruit.
- Les 30 presets chargent maintenant avec **reverb et delay à zéro et FX désactivés**. Le son doit rester sec à l'ouverture; les effets peuvent ensuite être activés avec `FX ENABLED` et réglés séparément.
- Les trois familles utilisent des réglages de base différents; les paramètres Osc B/FM/noise ajoutent une variation de source, pas seulement de l'espace.

## Limites et validation
Cette archive est une version source candidate. Elle n'a pas été compilée sous Windows dans cet environnement et n'a pas été écoutée dans FL Studio. Le moteur reste un synthé maison à oscillateurs, pas un moteur wavetable/FM commercial complet. Vérifier d'abord la compilation, puis les commandes et les presets dans FL Studio avant de remplacer un plugin déjà installé.

## Compilation
Utiliser le workflow Windows existant du dépôt, en conservant la branche de test et le workflow déjà connu comme fonctionnel.
