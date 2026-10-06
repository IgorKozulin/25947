# Подключение по SSH-ключу без пароля сервера

Инструкция предназначена для подключения к `ccfit.nsu.ru` с Windows, macOS и Linux.

> Подключение «без пароля» означает, что вместо пароля учётной записи сервер проверяет SSH-ключ. Сам закрытый ключ при этом может быть защищён локальной фразой-паролем.

## Перед началом

Понадобятся:

- логин на сервере, например `i.kozulin`;
- пароль от учётной записи — один раз, чтобы установить открытый ключ;
- SSH-клиент OpenSSH.

В примерах замените `ВАШ_ЛОГИН` своим логином.

Никому не передавайте закрытый ключ:

- Windows: `$HOME\.ssh\id_rsa_ccfit`;
- macOS/Linux: `~/.ssh/id_rsa_ccfit`.

Передавать на сервер можно только открытый ключ — файл с окончанием `.pub`.

---

## Windows 10/11

Команды выполняются в **PowerShell**. OpenSSH Client обычно уже установлен в современных версиях Windows.

Проверить наличие клиента:

```powershell
ssh -V
```

### 1. Создание отдельного ключа

```powershell
New-Item -ItemType Directory -Force "$HOME\.ssh" | Out-Null
ssh-keygen -t rsa -b 4096 -f "$HOME\.ssh\id_rsa_ccfit" -C "ccfit-windows"
```

При запросе `Enter passphrase` можно:

- задать фразу-пароль — более безопасный вариант;
- дважды нажать Enter и оставить её пустой.

Если программа сообщает, что файл уже существует, не перезаписывайте его, пока не убедитесь, что старый ключ больше нигде не используется.

### 2. Добавление открытого ключа на сервер

```powershell
Get-Content -Raw "$HOME\.ssh\id_rsa_ccfit.pub" | ssh ВАШ_ЛОГИН@ccfit.nsu.ru 'umask 077; mkdir -p ~/.ssh; chmod 700 ~/.ssh; cat >> ~/.ssh/authorized_keys; chmod 600 ~/.ssh/authorized_keys'
```

Введите пароль от учётной записи на сервере. При успешной настройке в дальнейшем он не понадобится.

### 3. Настройка SSH-клиента

Откройте или создайте файл конфигурации:

```powershell
notepad "$HOME\.ssh\config"
```

Добавьте:

```sshconfig
Host ccfit
    HostName ccfit.nsu.ru
    User ВАШ_ЛОГИН
    IdentityFile ~/.ssh/id_rsa_ccfit
    IdentitiesOnly yes
    PubkeyAcceptedAlgorithms +ssh-rsa
    HostkeyAlgorithms +ssh-rsa
```

Сохраните файл именно с именем `config`, без расширения `.txt`.

Параметры `PubkeyAcceptedAlgorithms` и `HostkeyAlgorithms` нужны для совместимости со старым SSH-сервером. Не добавляйте их в глобальную секцию `Host *`.

### 4. Проверка подключения

```powershell
ssh ccfit
```

---

## macOS и Linux

Команды выполняются в терминале.

### 1. Создание отдельного ключа

```bash
mkdir -p ~/.ssh
chmod 700 ~/.ssh
ssh-keygen -t rsa -b 4096 -f ~/.ssh/id_rsa_ccfit -C "ccfit-unix"
```

При запросе `Enter passphrase` можно задать локальную фразу-пароль либо дважды нажать Enter и оставить её пустой.

Если файл уже существует, не перезаписывайте его без проверки.

### 2. Добавление открытого ключа на сервер

```bash
cat ~/.ssh/id_rsa_ccfit.pub | ssh ВАШ_ЛОГИН@ccfit.nsu.ru \
'umask 077; mkdir -p ~/.ssh; chmod 700 ~/.ssh; cat >> ~/.ssh/authorized_keys; chmod 600 ~/.ssh/authorized_keys'
```

Введите пароль от учётной записи на сервере.

### 3. Настройка SSH-клиента

Откройте или создайте файл:

```bash
nano ~/.ssh/config
```

Добавьте:

```sshconfig
Host ccfit
    HostName ccfit.nsu.ru
    User ВАШ_ЛОГИН
    IdentityFile ~/.ssh/id_rsa_ccfit
    IdentitiesOnly yes
    PubkeyAcceptedAlgorithms +ssh-rsa
    HostkeyAlgorithms +ssh-rsa
```

На macOS также можно добавить:

```sshconfig
    AddKeysToAgent yes
    UseKeychain yes
```

Установите безопасные права:

```bash
chmod 600 ~/.ssh/config
```

Если на macOS ключ защищён фразой-паролем, его можно сохранить в Связке ключей:

```bash
ssh-add --apple-use-keychain ~/.ssh/id_rsa_ccfit
```

В Linux ключ можно добавить в работающий `ssh-agent`:

```bash
ssh-add ~/.ssh/id_rsa_ccfit
```

### 4. Проверка подключения

```bash
ssh ccfit
```

---

## Как понять, какой пароль запрашивается

Сообщение вида:

```text
Enter passphrase for key '...id_rsa_ccfit'
```

означает, что SSH запрашивает локальную фразу-пароль закрытого ключа. Это не пароль от сервера.

Сообщение вида:

```text
ВАШ_ЛОГИН@ccfit.nsu.ru's password:
```

означает, что сервер не принял ключ и перешёл к парольной аутентификации.

## Устранение неполадок

### Проверка с подробным журналом

Windows, macOS и Linux:

```text
ssh -vvv ccfit
```

В журнале важны строки:

```text
Offering public key
Server accepts key
Authentications that can continue
```

Закрытый ключ в журнале не выводится, но перед публикацией журнала желательно скрыть логин, IP-адреса и имена локальных файлов.

### Сервер продолжает запрашивать пароль

Войдите на сервер по паролю и проверьте права:

```bash
chmod go-w ~
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
ls -ld ~ ~/.ssh ~/.ssh/authorized_keys
```

Также убедитесь, что:

- ключ добавлен в учётную запись правильного пользователя;
- в `authorized_keys` находится содержимое файла `.pub`, а не закрытого ключа;
- весь ключ расположен на одной логической строке;
- файлы и каталог принадлежат пользователю, под которым выполняется вход;
- в `~/.ssh/config` указан правильный `IdentityFile`;
- секция начинается именно с `Host ccfit`, если подключение выполняется командой `ssh ccfit`.

### Ошибка `no mutual signature algorithm`

Проверьте наличие в секции `Host ccfit`:

```sshconfig
PubkeyAcceptedAlgorithms +ssh-rsa
HostkeyAlgorithms +ssh-rsa
```

Эти настройки разрешают устаревший алгоритм только для данного сервера. Для остальных серверов современные ограничения OpenSSH сохраняются.

### Ошибка в названии параметра

Очень старые версии клиента OpenSSH могут не знать параметр `PubkeyAcceptedAlgorithms`. В таком случае используйте:

```sshconfig
PubkeyAcceptedKeyTypes +ssh-rsa
```

### Слишком много попыток аутентификации

Ошибка `Too many authentication failures` обычно возникает, когда клиент перебирает много ключей. В конфигурации должны присутствовать:

```sshconfig
IdentityFile ~/.ssh/id_rsa_ccfit
IdentitiesOnly yes
```

---

## Подключение к другому серверу через ccfit

Успешный вход на `ccfit.nsu.ru` не означает, что ключ автоматически даёт доступ к другому серверу, например `10.4.0.68`. Это отдельное подключение с отдельной аутентификацией.

Не копируйте закрытый ключ на промежуточный сервер. Для безопасного сквозного подключения следует отдельно настроить `ProxyJump` либо использовать переадресацию агента только при полном доверии промежуточному серверу.

## Результат

После настройки подключение выполняется короткой командой:

```text
ssh ccfit
```

Если ключ не защищён фразой-паролем либо она сохранена локальным агентом/Связкой ключей, дополнительных запросов не будет.
