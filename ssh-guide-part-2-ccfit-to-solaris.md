# Часть 2. Подключение к Solaris через ccfit

Инструкция подходит для Windows, macOS и Linux.

Схема подключения:

```text
локальный компьютер → ccfit.nsu.ru → Solaris (10.4.0.68)
```

## Главное

Доступ `ccfit → Solaris` уже настроен на сервере. Создавать на ccfit новый SSH-ключ, переносить ключи и изменять `authorized_keys` в Solaris **не нужно**.

Пользователю требуется настроить только первый переход со своего компьютера на `ccfit.nsu.ru`. После входа на ccfit подключение к Solaris выполняется штатной командой:

```bash
ssh 10.4.0.68
```

Если имя пользователя в Solaris отличается от имени пользователя на ccfit:

```bash
ssh ЛОГИН_SOLARIS@10.4.0.68
```

## Предварительное условие

Должна быть завершена первая часть инструкции: команда ниже подключает локальный компьютер к ccfit по ключу:

```bash
ssh ccfit
```

В локальном SSH-конфиге должен существовать алиас `ccfit`. Например:

```sshconfig
Host ccfit
    HostName ccfit.nsu.ru
    User ВАШ_ЛОГИН
    IdentityFile ~/.ssh/id_rsa_ccfit
    IdentitiesOnly yes
    PubkeyAcceptedAlgorithms +ssh-rsa
    HostkeyAlgorithms +ssh-rsa
    ServerAliveInterval 60
    ServerAliveCountMax 3
```

Замените `ВАШ_ЛОГИН` своим логином.

---

## Вариант 1. Обычное подключение в два шага

Это основной и самый понятный способ. Он одинаков на всех операционных системах.

### Windows 10/11

Откройте PowerShell или Windows Terminal.

Подключитесь к ccfit:

```powershell
ssh ccfit
```

После появления приглашения командной строки ccfit выполните:

```bash
ssh 10.4.0.68
```

### macOS и Linux

Откройте терминал и выполните:

```bash
ssh ccfit
```

После входа на ccfit:

```bash
ssh 10.4.0.68
```

### Признак успешного входа

Solaris/illumos выводит баннер примерно такого вида:

```text
The Illumos Project     SunOS 5.11
```

После него появляется приглашение командной строки Solaris.

Чтобы вернуться из Solaris на ccfit:

```bash
exit
```

Чтобы затем вернуться с ccfit на локальный компьютер:

```bash
exit
```

---

## Вариант 2. Подключение одной командой

Можно сразу открыть Solaris через ccfit одной командой.

### Windows PowerShell / Windows Terminal

```powershell
ssh -t ccfit "ssh 10.4.0.68"
```

### macOS и Linux

```bash
ssh -t ccfit 'ssh 10.4.0.68'
```

Параметр `-t` создаёт интерактивный терминал на ccfit, внутри которого запускается штатное подключение к Solaris.

Это не `ProxyJump`: второй SSH-клиент запускается непосредственно на ccfit и использует уже настроенный там доступ к Solaris.

Если логин Solaris отличается:

```bash
ssh -t ccfit 'ssh ЛОГИН_SOLARIS@10.4.0.68'
```

---

## Вариант 3. Алиас `ssh solaris`

Чтобы каждый раз не вводить вложенную команду, можно создать локальный алиас. Никаких изменений на ccfit и Solaris при этом не производится.

### Windows

Откройте файл:

```powershell
notepad "$HOME\.ssh\config"
```

Добавьте отдельную секцию:

```sshconfig
Host solaris
    HostName ccfit.nsu.ru
    User ВАШ_ЛОГИН
    IdentityFile ~/.ssh/id_rsa_ccfit
    IdentitiesOnly yes
    PubkeyAcceptedAlgorithms +ssh-rsa
    HostkeyAlgorithms +ssh-rsa
    RequestTTY force
    RemoteCommand ssh 10.4.0.68
    ServerAliveInterval 60
    ServerAliveCountMax 3
```

Сохраните файл под именем `config` без расширения `.txt`.

### macOS и Linux

Откройте файл:

```bash
nano ~/.ssh/config
```

Добавьте:

```sshconfig
Host solaris
    HostName ccfit.nsu.ru
    User ВАШ_ЛОГИН
    IdentityFile ~/.ssh/id_rsa_ccfit
    IdentitiesOnly yes
    PubkeyAcceptedAlgorithms +ssh-rsa
    HostkeyAlgorithms +ssh-rsa
    RequestTTY force
    RemoteCommand ssh 10.4.0.68
    ServerAliveInterval 60
    ServerAliveCountMax 3
```

Установите права на конфигурацию:

```bash
chmod 600 ~/.ssh/config
```

### Проверка алиаса

На Windows, macOS или Linux выполните:

```bash
ssh solaris
```

Локальный SSH-клиент подключится к ccfit, после чего автоматически запустит на нём `ssh 10.4.0.68`.

> Этот алиас предназначен для интерактивного терминала. Он не является прямым сетевым доступом к Solaris и не подходит для `scp`, SFTP и VS Code Remote SSH.

---

## Что не нужно делать

Не выполняйте на ccfit повторную генерацию ключей без отдельной причины:

```bash
ssh-keygen
```

Не заменяйте файлы:

```text
~/.ssh/id_rsa
~/.ssh/authorized_keys
```

Не копируйте закрытые ключи между локальным компьютером, ccfit и Solaris.

Не добавляйте публичный ключ в `authorized_keys` на ccfit в попытке настроить переход ccfit → Solaris: `authorized_keys` управляет входящими подключениями к тому серверу, на котором находится файл.

## Устранение неполадок

### Пароль запрашивается при выполнении `ssh ccfit`

Проблема относится к первому переходу — локальный компьютер → ccfit. Проверьте первую часть инструкции и локальный файл `~/.ssh/config`.

Диагностика:

```bash
ssh -vvv ccfit
```

### Вход на ccfit работает, но `ssh 10.4.0.68` запрашивает пароль

Убедитесь, что команда выполняется уже внутри ccfit и используется правильный логин Solaris:

```bash
hostname
whoami
ssh -vvv 10.4.0.68
```

Не создавайте новый ключ и не перезаписывайте `authorized_keys`. Если штатный серверный доступ перестал работать, передайте администратору строки журнала вокруг:

```text
Offering public key
Server accepts key
Authentications that can continue
```

### Соединение с ccfit обрывается по бездействию

В локальной секции `Host ccfit` должны быть параметры:

```sshconfig
ServerAliveInterval 60
ServerAliveCountMax 3
```

### Предупреждение о ключе сервера

При первом подключении SSH может попросить подтвердить отпечаток узла. Сверьте отпечаток с данными администратора и только после этого подтвердите подключение.

Если появляется `REMOTE HOST IDENTIFICATION HAS CHANGED`, не удаляйте запись вслепую: сначала уточните у администратора, действительно ли ключ сервера был заменён.

## VS Code

Обычный терминал VS Code может использовать те же команды:

```bash
ssh ccfit
ssh 10.4.0.68
```

или одну команду:

```bash
ssh -t ccfit 'ssh 10.4.0.68'
```

Полноценный Microsoft VS Code Remote SSH устанавливает на целевой машине VS Code Server. Solaris/illumos им не поддерживается, поэтому успешный обычный SSH-вход не гарантирует работу Remote SSH.

## Итог

Никакая дополнительная настройка SSH на ccfit или Solaris не требуется.

Основной вариант:

```bash
ssh ccfit
ssh 10.4.0.68
```

Одной командой:

```bash
ssh -t ccfit 'ssh 10.4.0.68'
```

После создания локального алиаса:

```bash
ssh solaris
```
