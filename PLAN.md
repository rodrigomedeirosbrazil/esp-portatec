# Plano de Implementação da Feature de Acesso por PIN

Este documento detalha o plano para adicionar a funcionalidade de acesso por PIN ao dispositivo, incluindo a configuração de um Access Point Wi-Fi, uma interface web para entrada de PINs e a sincronização de PINs e tempo com um servidor externo.

---

## Fase 1: Configuração do Wi-Fi Access Point (AP) e Captive Portal

1.  **Alterar o modo do Wi-Fi:** O código principal (`src/main.cpp`) será modificado para que o ESP32 inicie em modo `WIFI_AP` (Access Point) com um SSID (nome da rede) definido (ex: "Portatec-Setup"). A rede será aberta, sem senha.
2.  **Implementar o Captive Portal:** Para que os usuários sejam redirecionados automaticamente para a página de controle, será configurado um servidor DNS no dispositivo. Todas as requisições de nomes serão respondidas com o próprio IP do ESP32, forçando a abertura da nossa página no navegador. Esta lógica será adicionada na classe `Webserver`.

---

## Fase 2: Desenvolvimento da Interface Web (Frontend) com Campo de PIN Avançado

1.  **Estrutura HTML do Modal:** O modal de PIN terá **seis campos de input individuais**, um para cada dígito, com `maxlength="1"`.
2.  **Estilo (CSS):** Adicionaremos CSS para estilizar os seis campos, garantindo uma apresentação clara e coesa.
3.  **Lógica Avançada com JavaScript:** Implementaremos um script para gerenciar a interação entre os campos:
    *   **Auto-Avanço:** Ao digitar um número, o foco pulará para o campo seguinte.
    *   **Auto-Retrocesso (Backspace):** Ao pressionar `Backspace` em um campo vazio, o foco e a ação de apagar retornarão ao campo anterior.
4.  **Submissão para o Servidor:** Um botão "Confirmar" enviará o PIN concatenado via `POST` para o endpoint `/open`.
5.  **Local do Código:** Todo o código de frontend (HTML, CSS, JS) será embarcado como strings no arquivo `src/Webserver/Webserver.cpp`.

---

## Fase 3: Gerenciamento de Tempo (Clock)

1.  **Nova Classe `Clock`:** Criaremos uma nova classe singleton (`src/Clock/Clock.h` e `src/Clock/Clock.cpp`) para gerenciar o tempo do dispositivo.
2.  **Mecanismo de Tempo:** Como o ESP8266 não possui um relógio de hardware (RTC), a classe `Clock` usará a seguinte estratégia:
    *   Armazenará um `timestamp` (época Unix) da última sincronização bem-sucedida com o servidor.
    *   Armazenará o valor de `millis()` no momento dessa sincronização.
    *   Fornecerá um método `now()` que calcula o tempo atual, somando o tempo decorrido (via `millis()`) ao `timestamp` da última sincronização.
3.  **Interface da Classe:** A classe terá um método `setTime(epoch)` para ser chamado pelo `Sync` e um método `now()` para ser consumido pelo `PinManager`.

---

## Fase 4: Gerenciamento de PINs (Lógica Interna)

1.  **Nova Classe `PinManager`:** Criaremos a classe `src/PinManager/PinManager.h` e `src/PinManager/PinManager.cpp`.
2.  **Integração com o `Clock`:** O `PinManager` dependerá da classe `Clock` para todas as operações de tempo.
3.  **Estrutura e Persistência do PIN:**
    *   **Estrutura do PIN:** Cada PIN será um objeto contendo: o código do PIN, um `timestamp` de **início** de validade e um `timestamp` de **fim** de validade (ambos em época Unix).
    *   **Armazenamento em Memória:** Manter uma lista (`std::vector`) de PINs em RAM para acesso rápido.
    *   **Armazenamento Persistente (Flash):** Utilizar o sistema de arquivos **LittleFS** para salvar a lista de PINs, garantindo que os dados persistam após reinicializações.
4.  **Responsabilidades do `PinManager`:**
    *   **Validação:** O método `isPinValid(String pin)` verificará se o PIN existe e se a data/hora atual (`Clock::getInstance().now()`) está **entre** o `timestamp` de início e o `timestamp` de fim.
    *   **Limpeza:** O método `purgeExpiredPins()` removerá os PINs cujo `timestamp` de fim já passou.
    *   **Exportação:** Fornecer um método para serializar a lista de PINs ativos em formato JSON, para ser enviada ao servidor.
    *   **Carga/Salvamento:** Ler o arquivo de PINs do LittleFS na inicialização e salvá-lo sempre que a lista for alterada pela sincronização.

---

## Fase 5: Sincronização Bidirecional com o Servidor

1.  **Adaptar a Classe `Sync`:** A classe `Sync` será o orquestrador da comunicação com o servidor.
2.  **Lógica de Sincronização:** O processo ocorrerá em passos:
    1.  **Envio do Estado Atual:** Periodicamente, a classe `Sync` obterá a lista de PINs ativos do `PinManager` e a enviará ao servidor em uma requisição **HTTP POST**, junto com o ID do dispositivo.
    2.  **Análise no Servidor:** O servidor comparará a lista recebida com sua lista mestra de PINs para aquele dispositivo.
    3.  **Resposta do Servidor:**
        *   **Se tudo estiver correto:** O servidor responderá com **HTTP 204 No Content**. O dispositivo não fará nada.
        *   **Se houver divergências:** O servidor responderá com **HTTP 200 OK** e o corpo da resposta conterá a **lista completa e corrigida** de PINs válidos (com data de início e fim), além do `timestamp` atual do servidor.
    4.  **Atualização no Dispositivo:** Ao receber uma resposta `200 OK`, o `Sync` irá:
        1.  Atualizar o relógio do dispositivo: `Clock::getInstance().setTime(serverEpoch)`.
        2.  Limpar a lista de PINs antiga no `PinManager`.
        3.  Adicionar a nova lista de PINs ao `PinManager`, que por sua vez a salvará no LittleFS.

---

## Fase 6: Integração, Segurança e Lógica Final

1.  **Novo Endpoint no `Webserver`:** Criaremos o endpoint `/open` na classe `Webserver`.
2.  **Fluxo de Abertura:** O endpoint chamará `PinManager::isPinValid()` para validar o PIN. Se for válido, o contador de tentativas é zerado e a ação de "abrir" é executada.
3.  **Mecanismo de Segurança e Feedback:**
    *   **Contador de Tentativas:** A classe `Webserver` terá um contador para tentativas incorretas.
    *   **PIN Incorreto:** Se o PIN for inválido, o sistema aguardará **4 segundos** antes de responder ao usuário.
    *   **Bloqueio por 3 Tentativas:** Após 3 tentativas incorretas, o Access Point será desativado e o dispositivo reiniciará.
4.  **Atualizações no `main.cpp`:** O `loop()` principal chamará periodicamente `PinManager::purgeExpiredPins()` e `Sync::syncPinsFromServer()`.

---

## Estrutura de Arquivos (Resumo das Mudanças):

*   **Novos Arquivos:**
    *   `src/Clock/Clock.h`
    *   `src/Clock/Clock.cpp`
    *   `src/PinManager/PinManager.h`
    *   `src/PinManager/PinManager.cpp`
*   **Arquivos a Modificar:**
    *   `src/main.cpp` (inicialização do LittleFS, configuração do modo AP, chamadas no loop)
    *   `src/Webserver/Webserver.h` e `src/Webserver/Webserver.cpp` (integração com `PinManager`, lógica de segurança)
    *   `src/Sync/Sync.h` e `src/Sync/Sync.cpp` (implementação da lógica de sincronização bidirecional)
    *   `platformio.ini` (para adicionar as dependências `ArduinoJson` e `LittleFS`, se necessário)

---

**Observação:** Embora este documento esteja em português, todo o código a ser implementado (incluindo comentários) será escrito em inglês.