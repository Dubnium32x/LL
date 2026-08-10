
##### DiSKOdev Presents
# Lincoln's Labyrinthine
#### Game Development Documentation

_Last Updated: 8/8/26_

## Table of Contents
1. [Introduction](#introduction)
2. [Game Overview](#game-overview)
3. [Characters and Story](#characters-and-story)
4. [Gameplay Mechanics](#gameplay-mechanics)
5. [Level Design](#level-design)
6. [Art and Visual Style](#art-and-visual-style)
7. [Sound and Music](#sound-and-music)
8. [Roles and Responsibilities](#roles-and-responsibilities)
9. [Tasks and Milestones](#tasks-and-milestones)

---

_1. Introduction_
---
## First Steps
If you are reading this, then you are involved in making the game for the Playdate called *Lincoln's Labyrinthine*! If this is the first time you are reading this, then welcome! This document serves as a living guideline in making the game possible. It will be updated as the game is developed, and it will serve as a reference for all team members. Please read this document carefully and refer to it often.

## About the Playdate
The Playdate is a handheld gaming console developed by Panic Inc. It features a unique design with a black-and-white screen, a crank for innovative gameplay mechanics, and a focus on delivering small, engaging games. The device has a 400x240 pixel display and supports both physical and digital game distribution.

We have managed to create an engine using the SDK and C API that allows us to create games for the Playdate. This engine will be used as the foundation for *Lincoln's Labyrinthine*, enabling us to leverage the Playdate's unique features and capabilities.

So far, we were able to even push the limitations further by allowing the system to play MOD tracker music. This takes up 15% power, but we believe it is worth the trouble, and fits the aesthetic well.

## About the Creator
As of writing this, the creator of the game is a single person, named DiSKOdev. He is a game developer and audio engineer with a passion for creating unique titles. He is not afraid of taking risks and pushing the boundaries of what is possible in game development. With a strong background in both programming and audio design, DiSKOdev brings a unique perspective to the development of *Lincoln's Labyrinthine*.

_2. Game Overview_
---
## Inspirations
Before diving into the concept, there is a few things that need to be addressed. 

For one, the game is massively inspired by the game called *Spooky's Jumpscare Mansion*. This game is a horror game that is as much of an endurance as it is intriguing. The game has a slight sense of humor, but is by no means safe for children. The horror aspects are incredible, and not just tied to jumpscares, despite its name. 

Another inspiration is Kirby's Adventure. Using this title as a reference, a lot of game mechanics can be derived from it, and can guarantee a fun and enjoyable experience. Gameplay-wise, we're certain that the simple controls will allow for a smooth and easy-to-pick-up experience. 

## Speedrunning
That said, we want this game to have the potential to have a speedrunning community. By having this benefit, we can ensure that the game will be long-lasting and able to have its own community. 

## Game Concept
At last, we can discuss what this game is about.

To put it briefly, *Lincoln's Labyrinthine* is a puzzle-platformer that challenges players to navigate through a series of increasingly complex mazes and obstacles. Players will control a long nosed, long eared bat named Lincoln, who has been through hard times, and is now stuck in his own shape-shifting home. 

As the game progresses, players will find that the labyrinth is full of secrets, and they will encounter various characters and artifacts that reveal the story of Lincoln's past. 

## Game Controls
Controlling Lincoln is simple, and the game expects that the player has a little understanding at least of how to use the Playdate and how to play a 2D platformer. Here are the expected controls:
- **D-Pad L/R**: Move Lincoln left or right.
    - Double tap to run.
- **D-Pad D**: Duck down.
    - Double tap to fall through certain platforms.
- **A Button**: Jump.
    - Tap again in midair to double jump.
- **B Button**: Interact with objects or characters.
    - This button also lets you attack using a weapon
    - Pressing D-Pad U + B will let you use an item.
- **Crank**: Use the crank to manipulate certain objects or mechanisms in the environment.
    - The crank can also be used to help Lincoln recover sanity.

## Game Features
A few things to cover with features first is to mention that the game has a few meters to watch.
- **Sanity Meter**: This meter represents Lincoln's mental state. At the start of a game, or on a save file, Lincoln's sanity will be at 50%. The meter will increase if his sanity is restored through meeting certain characters, being the light, or by any other means.
   - If Lincoln's sanity reaches 5% or lower, he will be in a state of panic, and will be unable to control his movements. This will make it difficult to navigate the labyrinth and avoid hazards.
- **Health Meter**: Of course, Lincoln is not invincible. The health meter will start with 100% and will decrease if Lincoln is hit by an enemy, instance, hazard, or any other way that can cause damage. Beware, as this health meter ties into the sanity meter. 
- **Stamina Meter**: Lincoln being able to run and jump, and even use the crank, will require stamina. This meter will start at 100% and will decrease as Lincoln performs actions that do extra work. You can recover stamina by standing still, walking, or using potions. Most of the time though, the stamina meter is not a concern. If you make one wrong move, going back to recover stamina can be a hassle, and can even lead to death if you are not careful.

As well as these meters, there are objects that can be collected, interacted, used, or even pushed. Here are some of these objects. Hopefully, this will provide the full list:

### Collectibles
- **Keys**: These are used to unlock doors, and are usually found in areas that require puzzles to be solved.
- **Notes**: These are scattered throughout the labyrinth and provide insight on Lincoln's past, the history of the characters, and whatever else. 
##### Note: You can start a new adventure with a finished save file, allowing anyone to recover notes and collectibles that were missed. This is a feature that will be implemented in the game, and will allow for a more complete experience.
- **Weapons**: Lincoln is able to hold one weapon at a time, and can use it to attack enemies or break certain obejcts. Some of these include brooms, hatchets, knives, or crosses. 
- **Smoke Bombs**: A smoke bomb will allow Lincoln to escape from instances, where if he i sbeing chased down, he can throw a smoke bomb, and escape the instance's line of sight.
- **Sanity Pill**: This item will restore Lincoln's sanity by 10%, and can be found in certain areas in the labyrinth. Use these sparingly, as they are not easy to find, and can be a lifesaver in certain situations.
- **Health Heart**: This item will restore Lincoln's health by 25%, and can be found in many areas in the game. Unlike potions, this item is instantly used, and can be collected by simply running into it. 

### Interactables
- **Doors**: These allow for progression to take place, and can be unlocked by keys, or simply opened if no key is required. Some doors may require a certain action to be performed, such as solving a puzzle or defeating an enemy.
- **Buttons**: These are one way switches that can be pressed to activate certain mechanisms. 
- **Levers**: These are two way switches that can be pulled to activate certain mechanisms, much like buttons.
- **Pressure Plates**: These switches can only be activated if there is something weighing down on them, such as a box or Lincoln himself. These will be important for puzzles later.
- **Crank Mechanisms**: Some puzzles will require the use of the Playdate's crank to manipulate certain objects or mechanisms in the environment. We figure that this will allow for a more unique experience.
- **Vents**: These are little hidding spots for Lincoln in case he needs to lose an instance. These vents can be found in certain areas, and can be used to escape from hazards and enemies.
- **Ladders and Ropes**: These allow Lincoln to climb up or down to reach different areas. Some ladders are there for decoration, but still very much interactable.
- **Springs**: These allow Lincoln to traverse higher to reach certain areas. Speaks for itself.
- **Sign**: Many signs are planted throughout the labyrinth, and can provide hints, tips, or even just a little nonsense.

### Pushables
- **Crates**: These can be pushed around to reach certain areas. You can also use these to weigh down pressure plates. 
- **Barrels**: These can be pushed around to reach certain areas, and can also be used to weigh down pressure plates. Some barrels may be explosive, and can be used to destroy certain objects or enemies.
- **Boulders**: These can be pushed around to reach certain areas. However, it takes a lot of stamina to push these. 

### Hazards
- **Spikes**: These are deadly hazards that will instantly kill Lincoln if he touches them. They can be found in many areas, if not most.
- **Bear Trap**: These are also deadly, but not an instant death. If you get stuck on one of these, you must mash the A button to escape. 
- **Pits**: These are deadly hazards that will instantly kill Lincoln if he falls into them. They can be found in many areas, if not most.

Okay, that's plenty for now. We'll cover more in the next sections, when we have a chance to go over characters, story, and level design.

_3. Characters and Story_
---

##### Disclaimer: This portion of the story is subject to change, but is also the introduction to the game.
### The Start of the Story

Coming home from his job, Lincoln the Long Earred Bat struggles to keep his composure as he passes by his fellow neighbors. Some wave and some smile. One of them trims the hedge on their lawn, and the clippers make Lincoln worry. He seems to find danger everywhere he looks. He is a nervous bat, and he is always looking over his shoulder. He is always looking for danger, and he is always looking for a way out.

He runs home, and realizes that being home was not as safe as he thought. But he felt comfortable.

He sat on his couch, and he felt safe enough. Hours fly by, and his clock strikes midnight. Lincoln walks into his bedroom, its dark, and a little dusty. It was unkept. Clothes, food wrappers, and beer bottles laid on the floor. He lays his back against the wall and looks at the corner of his room, where a tilted frame shows a picture of Lasse. Lasse has not been around for months. He thought he filed a police report, but ever since that day... his memory has gotten pretty hazy.

Images of laughter of him and his betrothed outside his porch looking at fireworks flash by. He picked up one of the beer bottles and throws it against the wall.

"Why is this happening to me??" he asks. He grabs his head. More flashing images of Lasse looking back at him. He begins to breathe heavily. Waddling across the room, losing his balance, he looks up, and it looks as if all the walls around him are beginning to expand.

He puts his hands down. Looking around, he asks again, "What is happening to me??"

He screams, again. "What is happening to me??"

He stumbles. He sees a glass of water by his desk. He takes a sip, and he hears moments of laughter, and the room begins to shrink.

Opening his eyes, he realizes that something is wrong. "No..." he continues, "Something is seriously wrong."

He goes to dial 911, but all that plays from the phone is only laughter. At first, it sounds... like the laugh he shared with Lasse. And as it continued, it became more and more unrecognizable.

"Whoever is doing this," he mutters, "Leave me alone. Get out of this house! Just leave me alone!"

Suddenly, a hand touches his shoulder. He jolts, looking behind him. Silence, and nothingness.

"Hello?" he asks, softly and nervously.

Then he clutches his fists, and asks again, aggressively. "Hello? Who is there?!"

A door opens behind him. The creaking, so it would seem, gave him the chills. The room no longer resembled anything normal. What he saw before him, was a skinny, long, narrow hallway. A dark, dark hallway. Walls too dark to see their detail. An old fashioned carpet painted in a pattern on the ground. The only thing that was audible, was the creaking of the door.

50 feet away, a light flickers above the door. Behind him was a wall, that was not there before. He turns around, and sees the wall. He looks back at the door, and it nearly... taunts him. Its presence is so strong, that it seems to be calling him. He takes a step forward, and the floor is creaking under his feet. He knows what he must do.

## Characters
- **Lincoln**: The main character of the game. A long nosed, long eared bat who is struggling with his sanity and trying to find his way out of the labyrinthine house. He is a nervous and anxious character, but he is also determined to find a way out.
- **Lasse**: Lincoln's betrothed, who has been missing for months.
- **Lincoln's Mind**: A representation of Lincoln's inner thoughts and fears, which will manifest in various ways throughout the game.

## Story Progression
You are basically going through this labyrinthine house trying to figure out where you are and trying to figure out what happened to your missing fiancee. In order to do this, you must explore the labyrinth and find all the notes, at least if you want 100% completion.

Lincoln's sanity sets the scene and makes him believe that something paranormal is happening. The story will be told through the notes, and the more you collect, the more the story is understood.

## Lincoln's Mind
As time went on, Lincoln's mind started to become lesser and lesser a part of him. He started to lose his grip on reality. Becoming so paranoid, that he started to believe that his fiancee was being held captive or something. When really... she was there the whole time. In 
the end, she tries to snap him out of it. 

## Ideas for the Story
These ideas are merely influenced or told subtly through the notes. 
### Idea #1
One idea is that Lincoln was part of a cult as a child. He thought that the paranormal was real because of it, and he ran away from it as he got older. He started to become too paranoid about it that he started to believe that the paranormal was real, and that he was being haunted by it. 

## Idea #2
Collecting orbs in the level, not because they are important, but doing so lights up a room as it dims. Going too dark will cause Lincoln to lose his sanity, and the room will become darker. The more orbs you collect, the more light is in the room, and the more sane Lincoln becomes.

## Idea #3
The game fucks with the player a lot when the sanity is low. Some ideas include:
- Fake crash screens that show the Playdate crash screen
- Fake pause menus
- Shrinking tiles
- Changing the controls to be reversed
- Changing the controls to be random
- Music is distorted and slowed down
- Certain objects in the level are
    - Invisible
    - Moving around
    - Changing their size
    - Becoming inverted or glitching
- Character becomes beta sprites
- The game will become darker when it shouldn't
- All lights in a level are off
- The image inverts
- The game will become a different game 
- All instances spawn at once
- The character gets bombarded with enemies
- Upon entry, the character is entirely different
- 