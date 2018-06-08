#pragma once

#include "LUwpUtilities\CustomPropertyProviderHelper.h"

namespace RedditQuick
{
	ref class App sealed : public ::Windows::UI::Xaml::Application
	{
	protected:
		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;

	internal:
		App();

	private:
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e);
	};

	// Class for a Reddit post
	ref class Topic sealed : Windows::UI::Xaml::Data::INotifyPropertyChanged,
		Windows::UI::Xaml::Data::ICustomPropertyProvider
	{
	public:
		Topic(Windows::Data::Json::JsonObject^ json);

		property Platform::String^ Title; // Title of the post
		property Platform::String^ Url; // URL the poster gives (example, image/video/youtube link)

		Windows::Foundation::Collections::IVector<Platform::Object^>^ LoadThread();

		DECLARE_ICUSTOMPROPERTYPROVIDER_VIRTUAL_METHODS
		virtual event Windows::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

	private:
		// Recursively collect the comments and replies from JSON data
		void AddComments(Windows::Foundation::Collections::IVector<Platform::Object^>^ comments, Windows::Data::Json::JsonObject^ json, int level);

		Platform::String^ permalink; // Relative path to the comment thread
	};

	// Class to encapsulate a subreddit's list of posts/topics/discussion thread_locals
	// This class provides an infinite scrolling list
	ref class TopicList sealed
	{
	public:
		TopicList(Platform::String^ subreddit, int sort);

		// These two methods are to allow for infinite loading via IncrementalLoadingBase
		Windows::Foundation::Collections::IVector<Platform::Object^>^ LoadMore(int count);
		bool HasMore();

	private:
		Platform::String^ after; // information to request more data
		Platform::String^ subreddit; // name of the subreddit
		int sort; // currently unused, intended to list by hot-ness or most-recent
	};

	// Class for a user comment within a discussion thread
	// We implement ICustomPropertyProvider to support data binding directly without XAML
	// The two (DataTemplate-bindable) properties are `Text` and `Indentation`.
	ref class Comment sealed : Windows::UI::Xaml::Data::INotifyPropertyChanged,
		Windows::UI::Xaml::Data::ICustomPropertyProvider
	{
	public:
		Comment(Platform::String^ text, int level);

		property Platform::String^ Text;
		property Windows::UI::Xaml::Thickness Indentation
		{
			Windows::UI::Xaml::Thickness get();
		}

		DECLARE_ICUSTOMPROPERTYPROVIDER_VIRTUAL_METHODS
		virtual event Windows::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

	internal:
		int level;
	};

	// Application main UI page
	ref class MainPage sealed : Windows::UI::Xaml::Controls::Page
	{
	public:
		MainPage();

	private:
		void OnBackPressed(Platform::Object^ sender, Windows::UI::Core::BackRequestedEventArgs^ e);

		void OnTopicClicked(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		void OnCommentClicked(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		void LoadSubReddit(Windows::UI::Xaml::Controls::AutoSuggestBox^ sender, Windows::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs^ args);

		Windows::UI::Xaml::Controls::AutoSuggestBox^ subredditTextBox;
		Windows::UI::Xaml::Controls::ListView^ topicsListView;
		Windows::UI::Xaml::Controls::ListView^ commentsListView;
		Windows::UI::Xaml::Controls::Grid^ topicGrid;
		TopicList^ topics;
	};
}
