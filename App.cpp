//#include <Windows.h> // for OutputDebugString; heavy => avoid
#include "LUwpUtilities\IncrementalLoadingList.h"
#include "LUwpUtilities\XamlHelper.h"
#include "LUwpUtilities\HttpHelper.h"
#include "LUwpUtilities\CustomPropertyBase.h"
#include "LUwpUtilities\CollectionHelper.h"
#include "LUwpUtilities\TaskHelper.h"

#include "App.h"

using namespace RedditQuick;
using namespace LUwpUtilities;

using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

App::App()
{
}

void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e)
{
	if (Windows::UI::Xaml::Window::Current->Content == nullptr)
	{
		Windows::UI::Xaml::Window::Current->Content = ref new MainPage();
	}
	Windows::UI::Xaml::Window::Current->Activate();
}

int __cdecl main(::Platform::Array<::Platform::String^>^ args)
{
	(void)args; // Unused parameter
	::Windows::UI::Xaml::Application::Start(ref new ::Windows::UI::Xaml::ApplicationInitializationCallback(
		[](::Windows::UI::Xaml::ApplicationInitializationCallbackParams^ p) {
		(void)p; // Unused parameter
		auto app = ref new App();
	}));
}

/// Implementation of `Topic` class

Topic::Topic(JsonObject^ json)
{
	auto td = json->GetNamedObject("data");
	//Use this code to check the JSON fields
	//OutputDebugString(json->ToString()->Data());
	//OutputDebugString(L"\n");
	Title = td->GetNamedString("title", nullptr);
	Url = td->GetNamedString("url", nullptr);
	permalink = td->GetNamedString("permalink", nullptr);
}

Windows::Foundation::Collections::IVector<Platform::Object^>^ Topic::LoadThread()
{
	auto url = "https://reddit.com" + permalink + ".json";
	auto response = Http::Get(url);
	auto text = Http::HttpResponseToText(response);
	auto json = JsonArray::Parse(text);
	auto result = CH::MakeObjectVector();
	for (auto it = json->First(); it->HasCurrent; it->MoveNext())
	{
		AddComments(result, it->Current->GetObject(), 0);
	}
	return result;
}

void Topic::AddComments(Windows::Foundation::Collections::IVector<Platform::Object^>^ comments, JsonObject^ json, int level)
{
	auto data = json->GetNamedObject("data", nullptr);

	if (data == nullptr)
		return;

	auto body = data->GetNamedString("body", nullptr);
	if (body != nullptr)
	{
		comments->Append(ref new Comment(body, level));
	}
	else
	{
		auto selftext = data->GetNamedString("selftext", nullptr);
		if (selftext != nullptr)
			comments->Append(ref new Comment(selftext, level));
	}

	auto children = data->GetNamedArray("children", nullptr);
	if (children != nullptr)
	{
		for (auto it = children->First(); it->HasCurrent; it->MoveNext())
		{
			if (it->Current->ValueType == JsonValueType::Object)
			{
				AddComments(comments, it->Current->GetObject(), level + 1);
			}
		}
	}

	// replies should be of type "Listing"
	auto replies = data->GetNamedValue("replies", nullptr);
	if (replies != nullptr)
	{
		switch (replies->ValueType)
		{
		case JsonValueType::Object:
			AddComments(comments, replies->GetObject(), level + 1);
			break;

		case JsonValueType::Array:
			for (auto it = replies->GetArray()->First(); it->HasCurrent; it->MoveNext())
			{
				if (it->Current->ValueType == JsonValueType::Object)
				{
					AddComments(comments, it->Current->GetObject(), level + 1);
				}
			}
			break;
		}
	}
}

IMPLEMENT_ICUSTOMPROPERTYPROVIDER_VIRTUAL_METHODS(Topic)
DECLARE_READONLY_CUSTOM_PROPERTY(Topic, Platform::String, Title)
DECLARE_READONLY_CUSTOM_PROPERTY(Topic, Platform::String, Url)
Windows::UI::Xaml::Data::ICustomProperty^ Topic::GetCustomProperty(Platform::String ^name)
{
	CHECK_AND_RETURN(Topic, Url)
	return TopicTitleProperty;
}

/// Implementation of `TopicList` class

TopicList::TopicList(Platform::String^ subreddit, int sort)
{
	this->subreddit = subreddit;
	this->sort = sort;
}

Windows::Foundation::Collections::IVector<Platform::Object^>^ TopicList::LoadMore(int count)
{
	auto url = "https://reddit.com/r/" + subreddit + "/.json";
	if (after != nullptr)
		url += "?after=" + after;

	auto response = Http::Get(url);
	auto text = Http::HttpResponseToText(response);
	auto json = JsonObject::Parse(text);
	auto data = json->GetNamedObject("data");
	after = data->GetNamedString("after", "");
	auto children = data->GetNamedArray("children");

	auto result = CH::MakeObjectVector();
	for (auto it = children->First(); it->HasCurrent; it->MoveNext())
	{
		auto tj = it->Current->GetObject();
		auto t = ref new Topic(tj);
		result->Append(t);
	}

	return result;
}

bool TopicList::HasMore()
{
	return (after == nullptr || after != "");
}

/// Implementation of `Comment` class

Comment::Comment(Platform::String^ text, int level)
{
	Text = text;
	this->level = level;
}

Windows::UI::Xaml::Thickness Comment::Indentation::get()
{
	Windows::UI::Xaml::Thickness t = { 5, 5, 5, 5 };
	t.Left += level * 10.0f;
	return t;
}

IMPLEMENT_ICUSTOMPROPERTYPROVIDER_VIRTUAL_METHODS(Comment)
DECLARE_READONLY_CUSTOM_PROPERTY(Comment, Platform::String, Text)
DECLARE_READONLY_CUSTOM_PROPERTY(Comment, Windows::UI::Xaml::Thickness, Indentation)
Windows::UI::Xaml::Data::ICustomProperty^ Comment::GetCustomProperty(Platform::String ^name)
{
	CHECK_AND_RETURN(Comment, Indentation)
	return CommentTextProperty;
}

/// Implementation of `MainPage` class

#define TOPIC_TEMPLATE L"<DataTemplate xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\">\
<StackPanel Orientation=\"Vertical\" Margin=\"5,5,5,5\">\
<TextBlock Text=\"{Binding Url}\" Foreground=\"blue\" FontSize=\"16\" Margin=\"5,0,5,0\" TextWrapping=\"Wrap\"/>\
<TextBlock Text=\"{Binding Title}\" FontSize=\"16\" Margin=\"5,0,5,0\" TextWrapping=\"Wrap\"/>\
</StackPanel></DataTemplate>"

#define COMMENT_TEMPLATE L"<DataTemplate xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\">\
<TextBlock Text=\"{Binding Text}\" FontSize=\"16\" Margin=\"{Binding Indentation}\" TextWrapping=\"Wrap\" MaxHeight=\"1000\" />\
</DataTemplate>"

MainPage::MainPage()
{
	topicsListView = XH::MakeListView(TOPIC_TEMPLATE, ref new ItemClickEventHandler(this, &MainPage::OnTopicClicked));

	commentsListView = XH::MakeListView(COMMENT_TEMPLATE, ref new ItemClickEventHandler(this, &MainPage::OnCommentClicked));

	subredditTextBox = ref new AutoSuggestBox();
	subredditTextBox->QueryIcon = ref new SymbolIcon(Symbol::Go);
	subredditTextBox->QuerySubmitted += ref new TypedEventHandler<AutoSuggestBox^, AutoSuggestBoxQuerySubmittedEventArgs^>(this, &MainPage::LoadSubReddit);

	auto mainGrid = XH::MakeGrid(1, 1);

	// The topic grid will be on the bottom
	topicGrid = XH::MakeGrid(2, 2);
	topicGrid->RowDefinitions->GetAt(0)->Height = GridLength(1, GridUnitType::Auto);
	topicGrid->ColumnDefinitions->GetAt(1)->Width = GridLength(1, GridUnitType::Auto);
	XH::AddToGrid(topicGrid, subredditTextBox, 0, 0);
	XH::AddToGrid(topicGrid, topicsListView, 1, 0);
	XH::AddToGrid(mainGrid, topicGrid, 0, 0);

	// Add the comment list on top of the topic grid
	XH::AddToGrid(mainGrid, commentsListView, 0, 0);
	XH::Collapse(commentsListView);

	this->Content = mainGrid;

	// Handle backpressed
	auto navman = Windows::UI::Core::SystemNavigationManager::GetForCurrentView();
	navman->AppViewBackButtonVisibility = AppViewBackButtonVisibility::Visible;
	navman->BackRequested += ref new EventHandler<BackRequestedEventArgs^>(this, &MainPage::OnBackPressed);
}

void MainPage::OnBackPressed(Platform::Object^ sender, Windows::UI::Core::BackRequestedEventArgs^ e)
{
	// Hide the comment list if it is visible, giving the effect of going back to the list of topics
	if (XH::IsVisible(commentsListView))
	{
		XH::Collapse(commentsListView);
		XH::MakeVisible(topicGrid);
		e->Handled = true;
		return;
	}
}

void MainPage::OnTopicClicked(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
	// Copy the post URL to clipboard so that user can paste to other applications
	auto content = ref new Windows::ApplicationModel::DataTransfer::DataPackage();
	content->SetText(((Topic^)e->ClickedItem)->Url);
	Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(content);

	// Now we load and show the post discussion thread
	TH::RunAsync(ref new ExecutionCallbackWithValue([](Platform::Object^ topic)
	{
		return ((Topic^)topic)->LoadThread();
	}), e->ClickedItem, ref new ExecutionCallback([=](Platform::Object^ comments)
	{
		XH::MakeVisible(commentsListView);
		// Make the topic grid invisible, necessary because the comment list has transparent background
		XH::Collapse(topicGrid);
		commentsListView->ItemsSource = comments;
	}));
}

void MainPage::OnCommentClicked(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
	// Copy user comment to clipboard so user can use the text
	auto content = ref new Windows::ApplicationModel::DataTransfer::DataPackage();
	content->SetText(((Comment^)e->ClickedItem)->Text);
	Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(content);
}

void MainPage::LoadSubReddit(Windows::UI::Xaml::Controls::AutoSuggestBox^ sender, Windows::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs^ args)
{
	auto subreddit = subredditTextBox->Text;
	topics = ref new TopicList(subreddit, 0);
	auto list = ref new IncrementalLoadingList(
		ref new LoadMoreItemsHandler(topics, &TopicList::LoadMore),
		ref new HasMoreItemsHandler(topics, &TopicList::HasMore)
	);
	topicsListView->ItemsSource = list;
}