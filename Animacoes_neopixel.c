if (caracter_press == '9') {
    letreiro();
    const char* ptr = message;
    while (*ptr) {
        clear_matrix(pio, sm);
        display_character(pio, sm, *ptr, color);
        sleep_ms(500);
        ptr++;
    }
    sleep_ms(1000); // Pausa de 1 segundo antes de reiniciar a mensagem
}
