# Changelog

## [Unreleased]

### Added
  * Show categories and series for the posts
  * [Utterances](https://utteranc.es/) comments support by @Jarijaas
  * design update:
    - improved instagram card style
    - readability and contrast updates
    - titles, tags, list of posts are not so dense as before
    - default colors were fine tuned
    - some small layout changes

### Changed
  * Configuration example for syntax highlight
  * Theme colors can be changed in a custom css file
  * minimal required Hugo version is v0.57.2
  * sort posts/pages on the error page by last modification date

### Fixed
  * fix breaking change in mainSections introduced in Hugo v0.57.0
  * deprecation warnings during site build with v0.55 and newer
  * optimize image size when viewed on mobile devices


## [1.1] - 2017-11-22
### Added
  * Theme colors can be changed in the config file
  * Makefile for easier development

### Changed
  * highlightjs was removed, builtin Chroma is used for syntax highlight
  * Position of the read more links is closer to the text
  * Abbreviations are used for social media names under the site title
  * Disqus localhost check was removed
  * Default archetype was extended and the new format is yaml

### Fixed
  * missing closing html tag


## [1.0.1] - 2017-08-24
### Fixed
  * Tag URLs by @Butt4cak3
  * gitlab social links by @CrazedProgrammer
  * gitlab url by @CrazedProgrammer


## [1.0] - 2017-04-22
### Added
  * Responsive minimalistic design
  * Syntax highlight with [highlight.js](https://highlightjs.org/)
  * [OpenGraph](http://ogp.me/) support
  * [Twitter cards](https://dev.twitter.com/cards/overview) support
  * [Disqus](https://disqus.com/) comments
  * [Google analytics](https://www.google.com/analytics/) (async)
  * Configrable pagination for posts
  * Lazy menu
  * Custom 404 page
