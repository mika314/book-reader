#include "app.hpp"
#include "imgui-impl-opengl3.h"
#include "imgui-impl-sdl.h"
#include "ui.hpp"
#include "utf8-parser.hpp"
#include <SDL_opengl.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <json-ser/json-ser.hpp>
#include <log/log.hpp>
#include <sstream>

static const std::string basePath = "/home/mika/Documents/belletristic/en/";

static auto updateUtf8Punctuation(std::string) -> std::string;

App::App(sdl::Window &aWindow, int /*argc*/, const char * /*argv*/[])
  : window(aWindow),
    gl_context(SDL_GL_CreateContext(window.get().get())),
    want([]() {
      SDL_AudioSpec ret;
      ret.freq = 24000;
      ret.format = AUDIO_S16;
      ret.channels = 1;
      ret.samples = 4096;
      return ret;
    }()),
    audio(nullptr, false, &want, &have, 0, [this](Uint8 *stream, int len) {
      for (auto i = 0; i < len / 2; ++i)
        reinterpret_cast<int16_t *>(stream)[i] = getWavSample();
      return len;
    })
{
  audio.pause(false);
  SDL_GL_MakeCurrent(window.get().get(), gl_context);
  SDL_GL_SetSwapInterval(1);

  // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char *glsl_version = "#version 100";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
  // GL 3.2 Core + GLSL 150
  const char *glsl_version = "#version 150";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                      SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window.get().get(), gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts
  // and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font
  // among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your
  // application (e.g. use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when
  // calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality
  // font rendering.
  // - Read 'docs/FONTS.md' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to
  // write a double backslash \\ !
  // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the
  // "fonts/" folder. See Makefile.emscripten for details. io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);

  ImGuiIO &io = ImGui::GetIO();
  // Optionally add more ranges depending on your needs
  static const ImWchar ranges[] = {
    0x0020,
    0x007f, // Basic Latin (this range covers the standard ASCII characters)

    0x00a0,
    0x00ff, // Latin-1 Supplement (for Western European languages)

    0x0400,
    0x052f, // Cyrillic (for Russian and similar languages)

    0x4e00,
    0x9faf, // CJK Ideograms (for Chinese and Japanese characters)

    0x2010,
    0x205e, // Punctuation including ‘’ “”
    0,
  };

  io.Fonts->AddFontFromFileTTF("Rajdhani-Regular.ttf", 30.f, nullptr, ranges);

  // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL,
  // io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);

  scanBooks();
}

auto App::tick() -> void
{
  // Poll and handle events (inputs, window resize, etc.)
  // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants
  // to use your inputs.
  // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application,
  // or clear/overwrite your copy of the mouse data.
  // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main
  // application, or clear/overwrite your copy of the keyboard data. Generally you may always pass
  // all inputs to dear imgui, and hide them from your application based on those two flags.
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    ImGui_ImplSDL2_ProcessEvent(&event);
    switch (event.type)
    {
    case SDL_QUIT: done = true; break;
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window.get().get()))
        done = true;
      break;
    }
  }

  // Start the Dear ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Save", "Ctrl+S"))
      {
        auto f = std::ofstream{"save.json"};
        jsonSer(f, save);
      }
      if (ImGui::MenuItem("Quit", "Ctrl+Q"))
        done = true;
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  if (ImGui::IsKeyReleased(ImGuiKey_S) && ImGui::GetIO().KeyCtrl)
  {
    auto f = std::ofstream{"save.json"};
    jsonSer(f, save);
  }
  if ((ImGui::IsKeyReleased(ImGuiKey_Q) && ImGui::GetIO().KeyCtrl))
    done = true;

  {
    auto window = Ui::Window("Books");
    processSearch();

    const auto listBoxHeight = ImGui::GetContentRegionAvail().y;
    if (ImGui::BeginListBox("Books List", ImVec2(-1, listBoxHeight)))
    {
      for (auto i = 0; i < static_cast<int>(tmpBooks.size()); ++i)
      {
        const auto is_selected = (selectedBook == i);
        if (ImGui::Selectable(tmpBooks[i], is_selected))
        {
          if (selectedBook != i)
          {
            selectedBook = i;
            loadBook(books[i]);
          }
        }

        // Set the initial focus when opening the listbox
        if (is_selected)
        {
          ImGui::SetItemDefaultFocus();
          if (needResetScroll)
          {
            ImGui::SetScrollHereY(0.5f); // Center the item vertically in the list box if possible
            needResetScroll = false;
          }
        }
      }
      ImGui::EndListBox();
    }
  }

  renderBook();

  ImGui::Render();
  glClearColor(.2f, .2f, .2f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  auto &io = ImGui::GetIO();

  // Update and Render additional Platform Windows
  // (Platform functions may change the current OpenGL context, so we save/restore it to make it
  // easier to paste this code elsewhere.
  //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
    SDL_Window *backup_current_window = SDL_GL_GetCurrentWindow();
    SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
  }

  window.get().glSwap();
}

auto App::renderBook() -> void
{
  if (selectedBook >= 0 && selectedBook < static_cast<int>(books.size()))
  {
    auto window = Ui::Window("Book");
    const auto &book = books[selectedBook];
    auto path = book.parent_path().string();
    if (path.find(basePath) == 0)
      path = path.substr(basePath.size());
    ImGui::Text("%s", path.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Copy"))
      ImGui::SetClipboardText(books[selectedBook].parent_path().c_str());

    if (!isReading)
    {
      if (ImGui::Button("Play"))
      {
        isReading = true;
        readingPos = -1;
      }
    }
    else
    {
      if (ImGui::Button("Stop"))
        isReading = false;
    }
    if (ImGui::IsKeyReleased(ImGuiKey_Space) && !searchActive)
    {
      if (!isReading)
      {
        isReading = true;
        readingPos = -1;
      }
      else
      {
        isReading = false;
      }
    }
    ImGui::SameLine();
    {
      ImGui::PushItemWidth(100);
      ImGui::SliderFloat("x##Reading Speed", &save.readingSpeed, 0.5f, 4.f, "%.2f");
      ImGui::PopItemWidth();
    }
    ImGui::SameLine();
    ImGui::Text("%s", book.stem().stem().c_str());
    const auto availableHeight = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("Scrolling",
                      ImVec2(0, availableHeight), // Use available height instead of fixed height
                      false,
                      0);
    ImGui::TextWrapped("%s", bookContent.c_str());
    const auto windowWidth = ImGui::GetWindowWidth();
    const auto scrollMax = ImGui::GetScrollMaxY();
    if (scrollMax <= 0)
    {
      ImGui::EndChild();
      return;
    }
    if (totalBookHeight < 0)
    {
      totalBookHeight = ImGui::CalcTextSize(bookContent.c_str(), nullptr, false, windowWidth).y;
      LOG("totalBookHeight:", totalBookHeight, "Scroll Max", scrollMax);
    }

    const auto MagicK = (scrollMax + availableHeight) / totalBookHeight;
    if (MagicK > 1.1 || MagicK < 0.9)
    {
      ImGui::EndChild();
      return;
    }
    if (ImGui::IsWindowHovered())
    {
      if (ImGui::IsMouseDown(0))
      {
        float scroll_delta = ImGui::GetIO().MouseDelta.y;
        lastScrollDelta = scroll_delta;
        ImGui::SetScrollY(ImGui::GetScrollY() - scroll_delta);
        desiredScroll = -1;
      }
      else
      {
        if (lastScrollDelta != 0)
        {
          desiredScroll = std::min(std::max(0.f, ImGui::GetScrollY() - 50 * lastScrollDelta), scrollMax);
          lastScrollDelta = 0;
        }
      }
    }

    if (readingPos < 0 && isReading)
    {
      const auto currentScrollY = ImGui::GetScrollY();
      auto startPos = 0;
      auto endPos = bookContent.size();
      while (endPos - startPos > 1)
      {
        const auto nextPos = startPos + (endPos - startPos) / 2;
        const auto heightAtPos =
          ImGui::CalcTextSize(bookContent.c_str(), bookContent.c_str() + nextPos, false, windowWidth).y *
          MagicK;
        if (heightAtPos < currentScrollY)
          startPos = nextPos;
        else
          endPos = nextPos;
      }
      readingPos = startPos;
      save.books[save.selectedBook].readingPos = readingPos;

      LOG("readingPos:", readingPos);
    }

    const auto isTalking = [&]() {
      audio.lock();
      auto res = wav.size() > 10000;
      audio.unlock();
      return res;
    }();

    const auto now = SDL_GetTicks();

    auto updateScrollBar = -1;

    if (needToUpdateScrollBar)
    {
      needToUpdateScrollBar = false;
      updateScrollBar = readingPos;
      LOG("updateScrollBar:", updateScrollBar);
    }

    if (isReading && !isTalking && cooldown < now)
    {
      updateScrollBar = readingPos;
      save.books[save.selectedBook].readingPos = readingPos;

      // skip all white space
      for (; readingPos < static_cast<int>(bookContent.size()) && isspace(bookContent[readingPos]);
           ++readingPos)
      {
      }
      auto sentence = std::string{};
      // read until \n
      for (; readingPos < static_cast<int>(bookContent.size()) &&
             ((bookContent[readingPos] != '\n' && bookContent[readingPos] != '.' &&
               bookContent[readingPos] != '?' && bookContent[readingPos] != '!') ||
              sentence.size() < 100);
           ++readingPos)
        sentence += bookContent[readingPos];
      if (readingPos < static_cast<int>(bookContent.size()))
      {
        sentence += bookContent[readingPos];
        ++readingPos;
      }

      cooldown = now + 200 + sentence.size() * 16;

      LOG(sentence);
      auto [s, w] = tts(updateUtf8Punctuation(std::move(sentence)), save.readingSpeed);
      wav.insert(wav.end(), std::begin(w), std::end(w));
    }

    if (updateScrollBar >= 0)
    {
      const auto heightAtPos =
        ImGui::CalcTextSize(
          bookContent.c_str(), bookContent.c_str() + updateScrollBar, false, windowWidth)
          .y *
        MagicK;
      desiredScroll = std::min(std::max(0.f, heightAtPos - 3 * ImGui::GetFontSize()), scrollMax);
      LOG("desiredScroll:", desiredScroll, "heightAtPos", heightAtPos, "MagicK", MagicK);
    }

    if (desiredScroll >= 0)
    {
      const auto currentScrollY = ImGui::GetScrollY();
      const auto diff = (desiredScroll - currentScrollY) * .05f;
      if (abs(diff) < .1f)
      {
        ImGui::SetScrollY(desiredScroll);
        desiredScroll = -1.f;
      }
      else if (abs(diff) < 1.f)
      {
        if (diff > 0)
          ImGui::SetScrollY(currentScrollY + 1);
        else
          ImGui::SetScrollY(currentScrollY - 1);
      }
      else
        ImGui::SetScrollY(currentScrollY + diff);
    }

    ImGui::EndChild();
  }
}

App::~App()
{
  auto f = std::ofstream{"save.json"};
  jsonSer(f, save);
}

auto App::scanBooks() -> void
{
  auto f = std::ifstream{"save.json"};
  if (f)
    jsonDeser(f, save);

  books.clear();
  for (auto &p : std::filesystem::recursive_directory_iterator(basePath))
  {
    std::ostringstream ss;
    ss << p.path();
    auto strPath = ss.str();
    if (![&]() {
          auto pos = strPath.find(".epub.txt");
          if (pos == strPath.size() - strlen(".epub.txt") - 1)
            return true;
          pos = strPath.find(".EPUB.TXT");
          if (pos == strPath.size() - strlen(".EPUB.TXT") - 1)
            return true;
          pos = strPath.find(".txt");
          if (pos == strPath.size() - strlen(".txt") - 1)
            return true;
          pos = strPath.find(".TXT");
          if (pos == strPath.size() - strlen(".TXT") - 1)
            return true;
          return false;
        }())
      continue;

    const auto bookName = p.path().stem().stem();
    if (bookName.empty())
    {
      LOG("empty book name:", p.path());
      continue;
    }
    save.books[bookName];
    books.push_back(p.path());
  }

  std::sort(std::begin(books), std::end(books));
  LOG("selected book", save.selectedBook);
  for (auto i = 0U; i < books.size(); ++i)
  {
    std::string bookName = books[i].stem().stem();
    if (save.selectedBook == bookName)
    {
      LOG("selectedBook:", books[i].stem().stem());
      selectedBook = i;
      loadBook(books[i]);
      break;
    }
  }
  tmpBooks.clear();
  tmpBooks2.clear();
  auto idx = 1;
  for (const auto &book : books)
  {
    std::ostringstream ss;
    ss << idx++ << ". " << book.stem().stem();
    if (ss.str().empty())
    {
      LOG("No title for book", book);
      continue;
    }
    tmpBooks2.push_back(ss.str());
  }
  for (const auto &book : tmpBooks2)
    tmpBooks.push_back(book.c_str());

  needResetScroll = true;
}

auto App::loadBook(const std::filesystem::path &v) -> void
{
  auto f = std::ifstream{v.string()};
  if (!f)
  {
    LOG("Failed to open file:", v.string());
    return;
  }
  auto ss = std::ostringstream{};
  ss << f.rdbuf();
  bookContent = ss.str();
  totalBookHeight = -1;
  isReading = false;
  readingPos = save.books[v.stem().stem()].readingPos;
  LOG("reading pos", readingPos);
  if (readingPos < 0 || readingPos >= static_cast<int>(bookContent.size()))
    readingPos = 0;
  save.selectedBook = v.stem().stem();
  isReading = false;
  needToUpdateScrollBar = true;
}

auto updateUtf8Punctuation(std::string str) -> std::string
{
  auto parser = Utf8Parser(std::move(str));
  auto r = std::string{};
  for (auto ch = parser.getCh(); !ch.empty(); ch = parser.getCh())
  {
    if (ch == "“" || ch == "”")
      r += "\"";
    else if (ch == "’")
      r += "\'";
    else if (ch == "—")
      r += "--";
    else if (ch == "…")
      r += "...";
    else if (ch.empty())
      continue;
    else if (ch[0] == '\0')
      continue;
    else if (ch.size() == 1)
      r += ch;
  }
  return r;
}

auto App::getWavSample() -> int16_t
{
  if (wav.empty())
    return 0;
  const auto r = wav.front();
  wav.pop_front();
  return r;
}

auto App::peekWavSample(int idx) -> int16_t
{
  if (idx >= static_cast<int>(wav.size()))
    return 0;
  return wav[idx];
}

auto App::processSearch() -> void
{
  auto search = [&](auto offset) {
    auto niddle = searchString;
    std::transform(std::begin(searchString), std::end(searchString), std::begin(niddle), ::tolower);

    for (auto i = 0U; i < tmpBooks2.size(); ++i)
    {
      const auto idx = static_cast<int>((i + std::max(0, selectedBook + offset)) % tmpBooks2.size());
      auto bookName = tmpBooks2[idx];
      std::transform(std::begin(bookName), std::end(bookName), std::begin(bookName), ::tolower);
      if (bookName.find(niddle) != std::string::npos)
      {
        if (selectedBook != idx)
        {
          selectedBook = idx;
          needResetScroll = true;
        }
        break;
      }
    }
  };

  if (ImGui::IsKeyReleased(ImGuiKey_F) && ImGui::GetIO().KeyCtrl)
  {
    if (!searchActive)
    {
      searchActive = true;
      searchString.clear();
      lastSelectedItem = selectedBook;
    }
    else
      search(1);
  }
  if (searchActive)
  {
    search(0);
    ImGui::SetKeyboardFocusHere();
    if (ImGui::InputText("Search", &searchString, ImGuiInputTextFlags_EnterReturnsTrue))
    {
      ImGui::SetKeyboardFocusHere();
      searchActive = false;
    }
    ImGui::Separator();
    if (ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
      if (selectedBook == -1)
      {
        selectedBook = lastSelectedItem;
        needResetScroll = true;
      }
      searchActive = false;
      loadBook(books[selectedBook]);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
      selectedBook = lastSelectedItem;
      needResetScroll = true;
      searchActive = false;
    }
  }
}
