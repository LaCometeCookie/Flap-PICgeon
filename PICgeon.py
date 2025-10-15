import pygame
import random
import sys

# === INITIALISATION ===
pygame.init()
WIDTH, HEIGHT = 400, 600
screen = pygame.display.set_mode((WIDTH, HEIGHT))
clock = pygame.time.Clock()
font = pygame.font.SysFont("Arial", 32)

# === CONSTANTES PHYSIQUES ===
gravity = 0.25
flap_strength = -6.5
pipe_gap = 150
pipe_speed = 3

# === VARIABLES DU JEU ===
bird_x = 80
bird_y = HEIGHT // 2
bird_velocity = 0

pipes = []  # liste de dictionnaires {"x":, "height":}
score = 0
ground_y = HEIGHT - 80
game_active = True

# === COULEURS ===
SKY = (135, 206, 250)
GROUND = (222, 184, 135)
PIPE_GREEN = (0, 180, 0)
BIRD_BODY = (255, 255, 0)
BIRD_BEAK = (255, 120, 0)
BIRD_EYE = (255, 255, 255)
BIRD_PUPIL = (0, 0, 0)
TEXT_COLOR = (255, 255, 255)

# === FONCTIONS ===
def create_pipe():
    """Crée un tuyau aléatoire."""
    height = random.randint(150, 400)
    return {"x": WIDTH, "height": height}

def move_pipes(pipes):
    """Déplace les tuyaux vers la gauche et supprime ceux hors écran."""
    for p in pipes:
        p["x"] -= pipe_speed
    return [p for p in pipes if p["x"] > -70]

def draw_pipes(pipes):
    """Dessine les tuyaux avec des rectangles verts."""
    for p in pipes:
        # Tuyau du bas
        pygame.draw.rect(screen, PIPE_GREEN, (p["x"], p["height"], 70, HEIGHT - p["height"]))
        # Tuyau du haut
        pygame.draw.rect(screen, PIPE_GREEN, (p["x"], 0, 70, p["height"] - pipe_gap))

def draw_bird(x, y, vel):
    """Dessine l'oiseau (cercle + bec + œil)."""
    # rotation approximée selon la vitesse
    angle = max(-30, min(30, -vel * 3))
    # corps
    pygame.draw.circle(screen, BIRD_BODY, (int(x), int(y)), 15)
    # bec
    pygame.draw.polygon(screen, BIRD_BEAK, [(x+15, y), (x+25, y-5), (x+25, y+5)])
    # œil
    pygame.draw.circle(screen, BIRD_EYE, (int(x+5), int(y-5)), 5)
    pygame.draw.circle(screen, BIRD_PUPIL, (int(x+5), int(y-5)), 2)

def check_collision(pipes):
    """Vérifie si l'oiseau touche un tuyau ou le sol."""
    global game_active
    bird_rect = pygame.Rect(bird_x-15, bird_y-15, 30, 30)
    for p in pipes:
        top_rect = pygame.Rect(p["x"], 0, 70, p["height"] - pipe_gap)
        bottom_rect = pygame.Rect(p["x"], p["height"], 70, HEIGHT - p["height"])
        if bird_rect.colliderect(top_rect) or bird_rect.colliderect(bottom_rect):
            game_active = False
    if bird_y >= ground_y - 15 or bird_y <= 0:
        game_active = False

def draw_ground():
    """Dessine le sol."""
    pygame.draw.rect(screen, GROUND, (0, ground_y, WIDTH, HEIGHT - ground_y))

def display_score(score):
    """Affiche le score."""
    text = font.render(f"Score: {score}", True, TEXT_COLOR)
    screen.blit(text, (10, 10))

# === EVENEMENT CYCLIQUE POUR LES TUYAUX ===
SPAWNPIPE = pygame.USEREVENT
pygame.time.set_timer(SPAWNPIPE, 1200)

# === BOUCLE PRINCIPALE ===
while True:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit()
        if event.type == pygame.KEYDOWN and game_active:
            if event.key == pygame.K_SPACE:
                bird_velocity = flap_strength
        if event.type == pygame.KEYDOWN and not game_active:
            if event.key == pygame.K_r:
                # reset
                bird_y = HEIGHT // 2
                bird_velocity = 0
                pipes = []
                score = 0
                game_active = True
        if event.type == SPAWNPIPE and game_active:
            pipes.append(create_pipe())

    # === LOGIQUE DU JEU ===
    if game_active:
        bird_velocity += gravity
        bird_y += bird_velocity
        pipes = move_pipes(pipes)
        check_collision(pipes)
        for p in pipes:
            if p["x"] + 70 == bird_x:
                score += 1

    # === DESSIN ===
    screen.fill(SKY)
    draw_pipes(pipes)
    draw_ground()
    draw_bird(bird_x, bird_y, bird_velocity)
    display_score(score)

    if not game_active:
        over_text = font.render("Game Over! Press R", True, (255, 50, 50))
        screen.blit(over_text, (WIDTH//2 - over_text.get_width()//2, HEIGHT//2))

    pygame.display.update()
    clock.tick(60)
