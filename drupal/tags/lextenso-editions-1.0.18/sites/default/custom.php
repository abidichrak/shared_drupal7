<?php

/** Have some complex redirection */

function custom_url_rewrite_outbound(&$path, &$options, $original_path) {
  // /eja/publisher/* -> /ouvrages/editeur/*
  // /eja/* -> /ouvrages/*
  if (preg_match('|^eja/publisher(/.*)|', $path, $matches)) {
    $path = 'ouvrages/editeur' . $matches[1];
  } elseif (preg_match('|^eja(/.*)|', $path, $matches)) {
    $path = 'ouvrages'. $matches[1];
  }
  // /search/eja/* -> /recherche/livres/*
  // /search/* -> /recherche/*
  if (preg_match('|^search/eja(/.*)|', $path, $matches)) {
    $path = 'recherche/livres'. $matches[1];
  } elseif (preg_match('|^search(/.*)|', $path, $matches)) {
    $path = 'recherche'. $matches[1];
  }
}

function custom_url_rewrite_inbound(&$result, $path, $path_language) {
  $isq = (@$_REQUEST['q'] == $path);
  $modified = false;
  // /eja/publisher/* <- /ouvrages/editeur/*
  // /eja/* <- /ouvrages/*
  if (preg_match('|^ouvrages/editeur(/.*)|', $result, $matches)) {
    $result = 'eja/publisher' . $matches[1];
    $modified = true;
  } elseif (preg_match('|^ouvrages(/.*)|', $result, $matches)) {
    $result = 'eja'. $matches[1];
    $modified = true;
  }
  // /search/eja/* <- /recherche/livres/*
  // /search/* <- /recherche/*
  if (preg_match('|^recherche/livres(/.*)|', $result, $matches)) {
    $result = 'search/eja'. $matches[1];
    $modified = true;
  } elseif (preg_match('|^recherche(/.*)|', $result, $matches)) {
    $result = 'search'. $matches[1];
    $modified = true;
  }
  // globalredirect module fails if we don't have this
  if ($isq && $modified)
    $_REQUEST['q'] = $result;
}

